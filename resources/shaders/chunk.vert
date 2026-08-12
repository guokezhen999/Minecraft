#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in float aCardinalLight;

out vec2 TexCoords;
out float CardinalLight;
out vec3 FragPos;

uniform mat4 projectionView;

void main()
{
    FragPos = aPos;
    gl_Position = projectionView * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
    CardinalLight = aCardinalLight;
}
