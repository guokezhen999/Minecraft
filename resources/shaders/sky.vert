#version 330 core

out vec2 UV;

void main()
{
    // Cover NDC with one oversized triangle (vertex IDs 0,1,2)
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(pos * 2.0 - 1.0, 1.0, 1.0);
    UV = gl_Position.xy * 0.5 + 0.5;
}
