#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in float CardinalLight;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;

uniform int underwater;
uniform int liquidPass;
uniform vec3 underwaterFogColor;
uniform float underwaterFogStart;
uniform float underwaterFogEnd;
uniform vec3 underwaterTint;

void main()
{
    vec4 color = texture(texture1, TexCoords);
    if (color.a == 0.0)
        discard;

    vec3 lit = color.rgb * CardinalLight;
    float dist = length(FragPos - cameraPos);
    float alpha = color.a;

    if (underwater != 0) {
        // Submerged: dense blue-green volume
        lit = mix(lit, underwaterTint, 0.55);
        if (liquidPass != 0) {
            // Water faces from below: darker, more opaque "ceiling/walls"
            lit = mix(lit, underwaterFogColor, 0.35);
            alpha = min(alpha + 0.45, 0.95);
        }
        float fogFactor = clamp(
            (underwaterFogEnd - dist) / (underwaterFogEnd - underwaterFogStart),
            0.0, 1.0);
        lit = mix(underwaterFogColor, lit, fogFactor);
    } else {
        // In air
        if (liquidPass != 0) {
            // Water surface from above: clearer cyan tint, moderate alpha
            lit = mix(lit, vec3(0.25, 0.55, 0.85), 0.35);
            alpha = min(alpha, 0.72);
        }
        float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
        lit = mix(fogColor, lit, fogFactor);
    }

    FragColor = vec4(lit, alpha);
}
