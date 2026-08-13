//
// Shared ortho HUD: colored quads, optional texture, ASCII bitmap text
//

#include "HudRenderer.h"
#include "../Shaders/Shader.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

#include "stb_image/stb_image.h"

namespace {

class HudShader : public Shader {
public:
    HudShader()
        : Shader("resources/shaders/hud.vert",
                 "resources/shaders/hud.frag") {
        GetUniforms();
    }

    void GetUniforms() override {}
};

} // namespace

HudRenderer::HudRenderer() {
    init();
}

HudRenderer::~HudRenderer() {
    if (m_fontTex)
        glDeleteTextures(1, &m_fontTex);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
}

void HudRenderer::init() {
    m_shader = new HudShader();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    loadFontTexture();
}

bool HudRenderer::loadFontTexture() {
    // OpenGL treats the first uploaded row as v=0 (bottom). Flip so PNG
    // row 0 (top of the atlas, ASCII 0–15) lands at v=1.
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load("resources/ui/font_ascii.png",
                                    &width, &height, &channels, STBI_rgb_alpha);
    stbi_set_flip_vertically_on_load(false);
    if (!data) {
        std::cerr << "[HudRenderer] Failed to load font_ascii.png: "
                  << stbi_failure_reason() << std::endl;
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &m_fontTex);
    glBindTexture(GL_TEXTURE_2D, m_fontTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

void HudRenderer::begin(int windowWidth, int windowHeight) {
    m_width = windowWidth;
    m_height = windowHeight;
    m_active = true;

    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                static_cast<float>(windowHeight), 0.0f,
                                -1.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader->Use();
    m_shader->SetMatrix4f("projection", proj);
    m_shader->SetInteger("texture1", 0);
    glBindVertexArray(m_vao);
}

void HudRenderer::end() {
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    m_active = false;
}

void HudRenderer::emitQuad(float x0, float y0, float x1, float y1,
                           float u0, float v0, float u1, float v1) {
    // y-down: (x0,y0) top-left, (x1,y1) bottom-right
    const float v[] = {
        x0, y0, u0, v0,
        x1, y0, u1, v0,
        x1, y1, u1, v1,
        x0, y0, u0, v0,
        x1, y1, u1, v1,
        x0, y1, u0, v1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void HudRenderer::drawQuad(float x0, float y0, float x1, float y1,
                           const glm::vec4& color) {
    if (!m_active)
        return;
    m_shader->SetInteger("useTexture", 0);
    m_shader->SetVector4f("color", color);
    emitQuad(x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, 1.0f);
}

void HudRenderer::drawTexturedQuad(float x0, float y0, float x1, float y1,
                                   float u0, float v0, float u1, float v1,
                                   const glm::vec4& color) {
    if (!m_active)
        return;
    m_shader->SetInteger("useTexture", 1);
    m_shader->SetVector4f("color", color);
    emitQuad(x0, y0, x1, y1, u0, v0, u1, v1);
}

float HudRenderer::textWidth(const std::string& str, float scale) const {
    return static_cast<float>(str.size()) * FONT_CELL * scale;
}

void HudRenderer::drawText(float x, float y, const std::string& str, float scale,
                           const glm::vec4& color) {
    if (!m_active || !m_fontTex || str.empty())
        return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fontTex);
    m_shader->SetInteger("useTexture", 1);
    m_shader->SetVector4f("color", color);

    const float gw = FONT_CELL * scale;
    const float gh = FONT_CELL * scale;
    float cx = x;
    for (unsigned char ch : str) {
        if (ch < 32 || ch > 126)
            ch = '?';
        const int col = ch % FONT_COLS;
        const int row = ch / FONT_COLS;
        const float u0 = static_cast<float>(col) / static_cast<float>(FONT_COLS);
        const float u1 = static_cast<float>(col + 1) / static_cast<float>(FONT_COLS);
        // After vertical flip on load: v=1 is atlas top (ASCII row 0)
        const float vTop = 1.0f - static_cast<float>(row) / static_cast<float>(FONT_COLS);
        const float vBot = 1.0f - static_cast<float>(row + 1) / static_cast<float>(FONT_COLS);
        emitQuad(cx, y, cx + gw, y + gh, u0, vTop, u1, vBot);
        cx += gw;
    }
}
