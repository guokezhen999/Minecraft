#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in float Shade;
in float SkyLevel;
in float BlockLevel;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float dayFactor;
uniform float ambient;

uniform int underwater;
uniform int liquidPass;
uniform vec3 underwaterFogColor;
uniform float underwaterFogStart;
uniform float underwaterFogEnd;
uniform vec3 underwaterTint;

float brightness(float t)
{
    return mix(0.12, 1.0, t * t * (2.0 - t));
}

void main()
{
    vec4 color = texture(texture1, TexCoords);
    if (color.a == 0.0)
        discard;

    float skyB = brightness(SkyLevel / 15.0);
    float blockB = brightness(BlockLevel / 15.0);
    float skyL = skyB * dayFactor;
    float light = max(skyL, blockB);
    vec3 lit = color.rgb * (ambient + (1.0 - ambient) * Shade * light);
    // Moonlight tint on sky-lit surfaces only; torches stay warm
    if (underwater == 0 && skyL >= blockB) {
        float night = 1.0 - smoothstep(0.42, 0.92, dayFactor);
        lit *= mix(vec3(1.0), vec3(0.90, 0.93, 1.05), night);
    }
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
            // Day: original cyan surface. Night: dark water, no bright haze.
            vec3 waterTint = mix(vec3(0.03, 0.05, 0.10), vec3(0.25, 0.55, 0.85),
                                 smoothstep(0.15, 0.75, dayFactor));
            lit = mix(lit, waterTint, 0.35);
            alpha = min(alpha, 0.72);
        }
        float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
        lit = mix(fogColor, lit, fogFactor);
    }

    FragColor = vec4(lit, alpha);
}
