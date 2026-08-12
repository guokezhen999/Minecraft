//
// Wireframe selection outline for the targeted block
//

#include "OutlineRenderer.h"
#include "../Camera.h"
#include "../Shaders/Shader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {

// Minimal solid-color shader (no texture)
class OutlineShader : public Shader {
public:
    OutlineShader()
        : Shader("resources/shaders/outline.vert",
                 "resources/shaders/outline.frag") {
        GetUniforms();
    }

    void GetUniforms() override {}
};

} // namespace

OutlineRenderer::OutlineRenderer() {
    init();
}

OutlineRenderer::~OutlineRenderer() {
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
}

void OutlineRenderer::init() {
    m_shader = new OutlineShader();

    // Unit cube [0,1]^3 edges as line list
    const float v[] = {
        // bottom
        0, 0, 0,  1, 0, 0,
        1, 0, 0,  1, 0, 1,
        1, 0, 1,  0, 0, 1,
        0, 0, 1,  0, 0, 0,
        // top
        0, 1, 0,  1, 1, 0,
        1, 1, 0,  1, 1, 1,
        1, 1, 1,  0, 1, 1,
        0, 1, 1,  0, 1, 0,
        // vertical
        0, 0, 0,  0, 1, 0,
        1, 0, 0,  1, 1, 0,
        1, 0, 1,  1, 1, 1,
        0, 0, 1,  0, 1, 1,
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

void OutlineRenderer::Render(const Camera& camera, const glm::ivec3& blockPos) {
    if (!m_shader)
        return;

    // Slightly inflate to reduce z-fighting with block faces
    constexpr float pad = 0.005f;
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(blockPos) - glm::vec3(pad));
    model = glm::scale(model, glm::vec3(1.0f + pad * 2.0f));

    m_shader->Use();
    m_shader->SetMatrix4f("projectionView", camera.GetProjectionViewMatrix());
    m_shader->SetMatrix4f("model", model);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // macOS often clamps glLineWidth to 1; still draw a bright outline
    glLineWidth(2.0f);

    glBindVertexArray(m_vao);

    // Outer dark pass then bright pass (readable on both light/dark faces)
    m_shader->SetVector3f("color", 0.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 24);
    m_shader->SetVector3f("color", 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 24);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}
