#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D texture1;
uniform vec4 color;
uniform int useTexture;

void main()
{
    if (useTexture != 0) {
        vec4 tex = texture(texture1, TexCoords);
        FragColor = tex * color;
        if (FragColor.a < 0.05)
            discard;
    } else {
        FragColor = color;
    }
}
