//
// Bottom-center hotbar HUD (slot frames + block icons + selection)
//

#ifndef MINECRAFT_HOTBARRENDERER_H
#define MINECRAFT_HOTBARRENDERER_H

#include <glad/glad.h>

class Hotbar;
class Shader;

class HotbarRenderer {
public:
    HotbarRenderer();
    ~HotbarRenderer();

    HotbarRenderer(const HotbarRenderer&) = delete;
    HotbarRenderer& operator=(const HotbarRenderer&) = delete;

    void Render(const Hotbar& hotbar, int windowWidth, int windowHeight);

private:
    void init();
    void drawQuad(float x0, float y0, float x1, float y1,
                  float u0, float v0, float u1, float v1);

    Shader* m_shader = nullptr;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};

#endif // MINECRAFT_HOTBARRENDERER_H
