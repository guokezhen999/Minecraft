#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aLight; // shade, sky, block

out vec2 TexCoords;
out float Shade;
out float SkyLevel;
out float BlockLevel;
out vec3 FragPos;

uniform mat4 projectionView;

void main()
{
    FragPos = aPos;
    gl_Position = projectionView * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
    Shade = aLight.x;
    SkyLevel = aLight.y;
    BlockLevel = aLight.z;
}
