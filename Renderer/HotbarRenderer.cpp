//
// Bottom-center hotbar HUD (slot frames + block icons + selection)
//

#include "HotbarRenderer.h"
#include "../UI/Hotbar.h"
#include "../Shaders/Shader.h"
#include "../World/Block/BlockDataBase.h"
#include "../World/Block/BlockId.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

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

HotbarRenderer::HotbarRenderer() {
    init();
}

HotbarRenderer::~HotbarRenderer() {
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
}

void HotbarRenderer::init() {
    m_shader = new HudShader();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // 6 verts × (x, y, u, v)
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void HotbarRenderer::drawQuad(float x0, float y0, float x1, float y1,
                              float u0, float v0, float u1, float v1) {
    // Two triangles, bottom-left origin (OpenGL HUD ortho)
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

void HotbarRenderer::Render(const Hotbar& hotbar, int windowWidth, int windowHeight) {
    if (!m_shader || windowWidth <= 0 || windowHeight <= 0)
        return;

    const float scale = std::max(1.0f, std::min(windowWidth, windowHeight) / 720.0f);
    const float slot = 44.0f * scale;
    const float gap = 4.0f * scale;
    const float pad = 10.0f * scale;
    const float border = 2.0f * scale;
    const float selectExtra = 3.0f * scale;
    const float iconInset = 6.0f * scale;
    const float bottomMargin = 14.0f * scale;

    const float totalW =
        Hotbar::SLOT_COUNT * slot + (Hotbar::SLOT_COUNT - 1) * gap;
    const float barX = (windowWidth - totalW) * 0.5f;
    const float barY = bottomMargin;

    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(windowWidth),
                                0.0f, static_cast<float>(windowHeight),
                                -1.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader->Use();
    m_shader->SetMatrix4f("projection", proj);
    m_shader->SetInteger("texture1", 0);

    glBindVertexArray(m_vao);

    // Backdrop behind all slots
    m_shader->SetInteger("useTexture", 0);
    m_shader->SetVector4f("color", 0.0f, 0.0f, 0.0f, 0.45f);
    drawQuad(barX - pad, barY - pad,
             barX + totalW + pad, barY + slot + pad,
             0.0f, 0.0f, 1.0f, 1.0f);

    auto& db = BlockDatabase::Get();
    db.atlas.Bind();

    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i) {
        const float x = barX + i * (slot + gap);
        const float y = barY;
        const bool selected = (i == hotbar.selectedIndex());

        // Slot plate
        m_shader->SetInteger("useTexture", 0);
        m_shader->SetVector4f("color", 0.18f, 0.18f, 0.18f, 0.85f);
        drawQuad(x, y, x + slot, y + slot, 0.0f, 0.0f, 1.0f, 1.0f);

        // Inner rim
        m_shader->SetVector4f("color", 0.08f, 0.08f, 0.08f, 0.95f);
        drawQuad(x + border, y + border,
                 x + slot - border, y + slot - border,
                 0.0f, 0.0f, 1.0f, 1.0f);

        // Selection highlight
        if (selected) {
            m_shader->SetVector4f("color", 1.0f, 1.0f, 1.0f, 0.95f);
            // Outer frame
            drawQuad(x - selectExtra, y - selectExtra,
                     x + slot + selectExtra, y,
                     0.0f, 0.0f, 1.0f, 1.0f);
            drawQuad(x - selectExtra, y + slot,
                     x + slot + selectExtra, y + slot + selectExtra,
                     0.0f, 0.0f, 1.0f, 1.0f);
            drawQuad(x - selectExtra, y,
                     x, y + slot,
                     0.0f, 0.0f, 1.0f, 1.0f);
            drawQuad(x + slot, y,
                     x + slot + selectExtra, y + slot,
                     0.0f, 0.0f, 1.0f, 1.0f);
        }

        const BlockId id = hotbar.slot(i);
        if (id == BlockId::Air)
            continue;

        const auto& data = db.GetData(id).GetBlockData();
        // Prefer side face; flora / uniform blocks look fine with side/all
        const auto uvs = db.atlas.GetTexture(data.texSideCoords);
        // Atlas order: (xMax,yMax), (xMin,yMax), (xMin,yMin), (xMax,yMin)
        const float uMin = uvs[2];
        const float vMin = uvs[5];
        const float uMax = uvs[0];
        const float vMax = uvs[1];

        const float ix0 = x + iconInset;
        const float iy0 = y + iconInset;
        const float ix1 = x + slot - iconInset;
        const float iy1 = y + slot - iconInset;

        m_shader->SetInteger("useTexture", 1);
        m_shader->SetVector4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
        // Flip V so atlas top matches screen up (atlas y grows downward in image space)
        drawQuad(ix0, iy0, ix1, iy1, uMin, vMax, uMax, vMin);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
