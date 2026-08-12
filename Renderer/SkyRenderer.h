//
// Procedural sky gradient (fullscreen; drawn before world)
//

#ifndef MINECRAFT_SKYRENDERER_H
#define MINECRAFT_SKYRENDERER_H

#include <glad/glad.h>

class Camera;
class Shader;

class SkyRenderer {
public:
    SkyRenderer();
    ~SkyRenderer();

    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    void Render(const Camera& camera, bool underwater = false);

private:
    void init();

    Shader* m_shader = nullptr;
    GLuint m_vao = 0;
};

#endif // MINECRAFT_SKYRENDERER_H
