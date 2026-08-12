#version 330 core

in vec2 UV;
out vec4 FragColor;

uniform mat4 invProjectionView;
uniform vec3 skyTop;
uniform vec3 skyHorizon;
uniform int underwater;
uniform vec3 underwaterColor;

void main()
{
    if (underwater != 0) {
        // Flat murky deep — no bright sky when submerged
        float band = smoothstep(0.0, 1.0, UV.y);
        FragColor = vec4(mix(underwaterColor * 0.65, underwaterColor, band), 1.0);
        return;
    }

    vec2 ndc = UV * 2.0 - 1.0;
    vec4 nearH = invProjectionView * vec4(ndc, -1.0, 1.0);
    vec4 farH  = invProjectionView * vec4(ndc,  1.0, 1.0);
    vec3 nearP = nearH.xyz / nearH.w;
    vec3 farP  = farH.xyz / farH.w;
    vec3 dir = normalize(farP - nearP);

    float t = smoothstep(-0.12, 0.55, dir.y);
    FragColor = vec4(mix(skyHorizon, skyTop, t), 1.0);
}
