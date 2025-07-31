#version 330 core

out vec4 FragColor;
in vec2 TexCoords;
in float CardinalLight;

uniform sampler2D texture1;

vec4 color;

void main()
{
    color = texture(texture1. TexCoords);
    FragColor = color * CardinalLight;
    if (FragColor.a == 0)
        discard;
}