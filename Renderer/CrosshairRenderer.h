//
// Screen-center crosshair (mouse is hidden while captured)
//

#ifndef MINECRAFT_CROSSHAIRRENDERER_H
#define MINECRAFT_CROSSHAIRRENDERER_H

#include <glad/glad.h>

class Shader;

class CrosshairRenderer {
public:
    CrosshairRenderer();
    ~CrosshairRenderer();

    CrosshairRenderer(const CrosshairRenderer&) = delete;
    CrosshairRenderer& operator=(const CrosshairRenderer&) = delete;

    void Render(int windowWidth, int windowHeight);

private:
    void init();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    Shader* m_shader = nullptr;
};

#endif //MINECRAFT_CROSSHAIRRENDERER_H
