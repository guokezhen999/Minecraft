#version 330 core

in vec2 UV;
out vec4 FragColor;

uniform mat4 invProjectionView;
uniform vec3 skyTop;
uniform vec3 skyHorizon;
uniform int underwater;
uniform vec3 underwaterColor;
uniform vec3 sunDir;
uniform vec3 sunDiscColor;
uniform vec3 moonDir;
uniform vec3 moonDiscColor;

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
    vec3 color = mix(skyHorizon, skyTop, t);

    float muSun = dot(dir, sunDir);
    if (sunDir.y > -0.08) {
        float disc = smoothstep(0.99945, 0.99982, muSun);
        float glow = pow(clamp(muSun, 0.0, 1.0), 28.0);
        float halo = pow(clamp(muSun, 0.0, 1.0), 6.0);
        color += sunDiscColor * (disc * 3.2 + glow * 0.50 + halo * 0.10);
    }

    float muMoon = dot(dir, moonDir);
    if (moonDir.y > 0.02) {
        float disc = smoothstep(0.99970, 0.99990, muMoon);
        float glow = pow(clamp(muMoon, 0.0, 1.0), 40.0);
        color += moonDiscColor * (disc * 1.35 + glow * 0.18);
    }

    FragColor = vec4(color, 1.0);
}
