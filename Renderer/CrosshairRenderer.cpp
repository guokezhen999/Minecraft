//
// Screen-center crosshair (mouse is hidden while captured)
//

#include "CrosshairRenderer.h"
#include "../Shaders/Shader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {

class CrosshairShader : public Shader {
public:
    CrosshairShader()
        : Shader("resources/shaders/outline.vert",
                 "resources/shaders/outline.frag") {
        GetUniforms();
    }

    void GetUniforms() override {}
};

} // namespace

CrosshairRenderer::CrosshairRenderer() {
    init();
}

CrosshairRenderer::~CrosshairRenderer() {
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
}

void CrosshairRenderer::init() {
    m_shader = new CrosshairShader();

    // Two line segments forming a + in local pixel space (centered at origin)
    constexpr float arm = 10.0f;
    const float v[] = {
        -arm, 0.0f, 0.0f,   arm, 0.0f, 0.0f,
         0.0f, -arm, 0.0f,  0.0f,  arm, 0.0f,
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

void CrosshairRenderer::Render(int windowWidth, int windowHeight) {
    if (!m_shader || windowWidth <= 0 || windowHeight <= 0)
        return;

    // Pixel → NDC orthographic, origin at screen center
    const float halfW = windowWidth * 0.5f;
    const float halfH = windowHeight * 0.5f;
    glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, -1.0f, 1.0f);
    glm::mat4 model(1.0f);

    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    m_shader->Use();
    m_shader->SetMatrix4f("projectionView", proj);
    m_shader->SetMatrix4f("model", model);

    glBindVertexArray(m_vao);
    m_shader->SetVector3f("color", 0.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 4);
    // Slightly smaller bright pass so the + has a dark rim
    m_shader->SetMatrix4f("model", glm::scale(glm::mat4(1.0f), glm::vec3(0.85f)));
    m_shader->SetVector3f("color", 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}
