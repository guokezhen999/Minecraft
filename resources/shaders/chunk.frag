#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in float Shade;
in float SkyLevel;
in float BlockLevel;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float dayFactor;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 moonDir;
uniform vec3 moonColor;
uniform vec3 skyLightColor;
uniform int celestial;

uniform int underwater;
uniform int liquidPass;
uniform vec3 underwaterFogColor;
uniform float underwaterFogStart;
uniform float underwaterFogEnd;
uniform vec3 underwaterTint;

const vec3 TORCH = vec3(1.00, 0.80, 0.52);

float brightness(float t)
{
    return mix(0.12, 1.0, t * t * (2.0 - t));
}

void main()
{
    vec4 color = texture(texture1, TexCoords);
    if (color.a == 0.0)
        discard;

    vec3 n = normalize(Normal);
    float skyB = brightness(SkyLevel / 15.0);
    float blockB = brightness(BlockLevel / 15.0);

    vec3 light;
    if (celestial != 0) {
        float sunN = max(dot(n, sunDir), 0.0);
        float moonN = max(dot(n, moonDir), 0.0);
        if (abs(n.y) < 0.15 && abs(n.x) > 0.35 && abs(n.z) > 0.35) {
            sunN = 0.35 + 0.65 * abs(dot(n, sunDir));
            moonN = 0.35 + 0.65 * abs(dot(n, moonDir));
        }
        float hemi = mix(0.50, 1.0, clamp(n.y * 0.5 + 0.5, 0.0, 1.0));
        vec3 daylight = skyB * (skyLightColor * hemi + sunColor * sunN + moonColor * moonN);
        light = max(daylight, vec3(blockB) * TORCH);
    } else {
        float cardinal = 0.82;
        if (abs(n.y) > 0.7)
            cardinal = n.y > 0.0 ? 1.00 : 0.72;
        else if (abs(n.x) > 0.7)
            cardinal = 0.90;
        float skyL = skyB * dayFactor;
        light = vec3(max(skyL, blockB) * cardinal);
    }
    vec3 lit = color.rgb * (Shade * light);
    if (celestial == 0 && underwater == 0) {
        float skyL = skyB * dayFactor;
        if (skyL >= blockB) {
            float night = 1.0 - smoothstep(0.42, 0.92, dayFactor);
            lit *= mix(vec3(1.0), vec3(0.90, 0.93, 1.05), night);
        }
    }

    float dist = length(FragPos - cameraPos);
    float alpha = color.a;

    if (underwater != 0) {
        lit = mix(lit, underwaterTint, 0.55);
        if (liquidPass != 0) {
            lit = mix(lit, underwaterFogColor, 0.35);
            alpha = min(alpha + 0.45, 0.95);
        }
        float fogFactor = clamp(
            (underwaterFogEnd - dist) / (underwaterFogEnd - underwaterFogStart),
            0.0, 1.0);
        lit = mix(underwaterFogColor, lit, fogFactor);
    } else {
        if (liquidPass != 0) {
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
