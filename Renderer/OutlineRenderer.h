//
// Wireframe selection outline for the targeted block
//

#ifndef MINECRAFT_OUTLINERENDERER_H
#define MINECRAFT_OUTLINERENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

class Camera;
class Shader;

class OutlineRenderer {
public:
    OutlineRenderer();
    ~OutlineRenderer();

    OutlineRenderer(const OutlineRenderer&) = delete;
    OutlineRenderer& operator=(const OutlineRenderer&) = delete;

    void Render(const Camera& camera, const glm::ivec3& blockPos);

private:
    void init();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    Shader* m_shader = nullptr;
};

#endif //MINECRAFT_OUTLINERENDERER_H
