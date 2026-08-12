//
// Procedural sky gradient (fullscreen; drawn before world)
//

#include "SkyRenderer.h"
#include "../Camera.h"
#include "../Shaders/Shader.h"
#include "../World/WorldConstants.h"

namespace {

class SkyShader : public Shader {
public:
    SkyShader()
        : Shader("resources/shaders/sky.vert",
                 "resources/shaders/sky.frag") {
        GetUniforms();
    }

    void GetUniforms() override {}
};

} // namespace

SkyRenderer::SkyRenderer() {
    init();
}

SkyRenderer::~SkyRenderer() {
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
}

void SkyRenderer::init() {
    m_shader = new SkyShader();
    // Attribute-less draw; VAO still required on core profile
    glGenVertexArrays(1, &m_vao);
}

void SkyRenderer::Render(const Camera& camera, bool underwater) {
    if (!m_shader)
        return;

    const glm::mat4 invPV = glm::inverse(camera.GetProjectionViewMatrix());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    m_shader->Use();
    m_shader->SetMatrix4f("invProjectionView", invPV);
    m_shader->SetInteger("underwater", underwater ? 1 : 0);
    m_shader->SetVector3f("skyTop", SKY_TOP_R, SKY_TOP_G, SKY_TOP_B);
    m_shader->SetVector3f("skyHorizon", SKY_HORIZON_R, SKY_HORIZON_G, SKY_HORIZON_B);
    m_shader->SetVector3f("underwaterColor",
                          UNDERWATER_FOG_R, UNDERWATER_FOG_G, UNDERWATER_FOG_B);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}
