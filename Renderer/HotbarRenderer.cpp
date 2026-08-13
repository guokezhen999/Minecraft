//
// Bottom-center hotbar HUD (slot frames + block icons + selection)
//

#include "HotbarRenderer.h"
#include "HudRenderer.h"
#include "../UI/Hotbar.h"
#include "../World/Block/BlockDataBase.h"
#include "../World/Block/BlockId.h"

#include <algorithm>
#include <glm/glm.hpp>

void HotbarRenderer::Render(HudRenderer& hud, const Hotbar& hotbar,
                            int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0)
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
    // HudRenderer origin is top-left
    const float barY = windowHeight - bottomMargin - slot;

    hud.drawQuad(barX - pad, barY - pad,
                 barX + totalW + pad, barY + slot + pad,
                 {0.0f, 0.0f, 0.0f, 0.45f});

    auto& db = BlockDatabase::Get();
    db.atlas.Bind();

    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i) {
        const float x = barX + i * (slot + gap);
        const float y = barY;
        const bool selected = (i == hotbar.selectedIndex());

        hud.drawQuad(x, y, x + slot, y + slot, {0.18f, 0.18f, 0.18f, 0.85f});
        hud.drawQuad(x + border, y + border,
                     x + slot - border, y + slot - border,
                     {0.08f, 0.08f, 0.08f, 0.95f});

        if (selected) {
            const glm::vec4 hi{1.0f, 1.0f, 1.0f, 0.95f};
            hud.drawQuad(x - selectExtra, y - selectExtra,
                         x + slot + selectExtra, y, hi);
            hud.drawQuad(x - selectExtra, y + slot,
                         x + slot + selectExtra, y + slot + selectExtra, hi);
            hud.drawQuad(x - selectExtra, y, x, y + slot, hi);
            hud.drawQuad(x + slot, y, x + slot + selectExtra, y + slot, hi);
        }

        const BlockId id = hotbar.slot(i);
        if (id == BlockId::Air)
            continue;

        const auto& data = db.GetData(id).GetBlockData();
        const auto uvs = db.atlas.GetTexture(data.texSideCoords);
        const float uMin = uvs[2];
        const float vMin = uvs[5];
        const float uMax = uvs[0];
        const float vMax = uvs[1];

        const float ix0 = x + iconInset;
        const float iy0 = y + iconInset;
        const float ix1 = x + slot - iconInset;
        const float iy1 = y + slot - iconInset;

        db.atlas.Bind();
        // Flip V so atlas top matches screen up (atlas y grows downward in image space)
        hud.drawTexturedQuad(ix0, iy0, ix1, iy1, uMin, vMin, uMax, vMax,
                             {1.0f, 1.0f, 1.0f, 1.0f});
    }
}
