//
// Bottom-center hotbar HUD (slot frames + block icons + selection)
// Inventory catalog: click a block onto the selected hotbar slot
//

#include "HotbarRenderer.h"
#include "HudRenderer.h"
#include "../UI/Hotbar.h"
#include "../UI/Inventory.h"
#include "../UI/Menu.h"
#include "../World/Block/BlockDataBase.h"
#include "../World/Block/BlockId.h"

#include <algorithm>
#include <glm/glm.hpp>

namespace {

struct BarGeom {
    float slot = 44.0f;
    float gap = 4.0f;
    float border = 2.0f;
    float iconInset = 6.0f;
    float selectExtra = 3.0f;
    float barX = 0.0f;
    float barY = 0.0f;
};

BarGeom barGeom(int windowWidth, int windowHeight) {
    BarGeom g;
    const float scale = std::max(1.0f, std::min(windowWidth, windowHeight) / 720.0f);
    g.slot = 44.0f * scale;
    g.gap = 4.0f * scale;
    g.border = 2.0f * scale;
    g.iconInset = 6.0f * scale;
    g.selectExtra = 3.0f * scale;
    const float pad = 10.0f * scale;
    const float bottomMargin = 14.0f * scale;
    const float totalW =
        Hotbar::SLOT_COUNT * g.slot + (Hotbar::SLOT_COUNT - 1) * g.gap;
    g.barX = (windowWidth - totalW) * 0.5f;
    g.barY = windowHeight - bottomMargin - g.slot;
    (void)pad;
    return g;
}

void drawBlockIcon(HudRenderer& hud, float x, float y, float slot, float inset,
                   BlockId id) {
    if (id == BlockId::Air)
        return;
    auto& db = BlockDatabase::Get();
    const auto& data = db.GetData(id).GetBlockData();
    const auto uvs = db.atlas.GetTexture(data.texSideCoords);
    const float uMin = uvs[2];
    const float vMin = uvs[5];
    const float uMax = uvs[0];
    const float vMax = uvs[1];
    db.atlas.Bind();
    hud.drawTexturedQuad(x + inset, y + inset, x + slot - inset, y + slot - inset,
                         uMin, vMin, uMax, vMax, {1.0f, 1.0f, 1.0f, 1.0f});
}

void drawSlot(HudRenderer& hud, const BarGeom& g, float x, float y,
              bool selected, BlockId id) {
    hud.drawQuad(x, y, x + g.slot, y + g.slot, {0.18f, 0.18f, 0.18f, 0.85f});
    hud.drawQuad(x + g.border, y + g.border,
                 x + g.slot - g.border, y + g.slot - g.border,
                 {0.08f, 0.08f, 0.08f, 0.95f});
    if (selected) {
        const glm::vec4 hi{1.0f, 1.0f, 1.0f, 0.95f};
        hud.drawQuad(x - g.selectExtra, y - g.selectExtra,
                     x + g.slot + g.selectExtra, y, hi);
        hud.drawQuad(x - g.selectExtra, y + g.slot,
                     x + g.slot + g.selectExtra, y + g.slot + g.selectExtra, hi);
        hud.drawQuad(x - g.selectExtra, y, x, y + g.slot, hi);
        hud.drawQuad(x + g.slot, y, x + g.slot + g.selectExtra, y + g.slot, hi);
    }
    drawBlockIcon(hud, x, y, g.slot, g.iconInset, id);
}

} // namespace

void HotbarRenderer::Render(HudRenderer& hud, const Hotbar& hotbar,
                            int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0)
        return;

    const BarGeom g = barGeom(windowWidth, windowHeight);
    const float scale = std::max(1.0f, std::min(windowWidth, windowHeight) / 720.0f);
    const float pad = 10.0f * scale;
    const float totalW =
        Hotbar::SLOT_COUNT * g.slot + (Hotbar::SLOT_COUNT - 1) * g.gap;

    hud.drawQuad(g.barX - pad, g.barY - pad,
                 g.barX + totalW + pad, g.barY + g.slot + pad,
                 {0.0f, 0.0f, 0.0f, 0.45f});

    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i) {
        const float x = g.barX + i * (g.slot + g.gap);
        drawSlot(hud, g, x, g.barY, i == hotbar.selectedIndex(), hotbar.slot(i));
    }
}

void HotbarRenderer::RenderInventory(HudRenderer& hud, Hotbar& hotbar, MenuContext& ctx) {
    if (!ctx.hud || ctx.width <= 0 || ctx.height <= 0)
        return;

    hud.drawQuad(0.0f, 0.0f,
                 static_cast<float>(ctx.width), static_cast<float>(ctx.height),
                 {0.0f, 0.0f, 0.0f, 0.40f});

    const BarGeom g = barGeom(ctx.width, ctx.height);
    const float scale = ctx.uiScale;
    const float cell = 48.0f * scale;
    const float gap = 6.0f * scale;
    const int cols = Inventory::COLS;
    const int rows = (Inventory::COUNT + cols - 1) / cols;
    const float gridW = cols * cell + (cols - 1) * gap;
    const float gridH = rows * cell + (rows - 1) * gap;
    const float titleH = 28.0f * scale;
    const float hintH = 22.0f * scale;
    const float pad = 16.0f * scale;
    const float panelW = gridW + pad * 2.0f;
    const float panelH = gridH + pad * 2.0f + titleH + hintH;
    const float panelX = (ctx.width - panelW) * 0.5f;
    const float panelY = (ctx.height - panelH) * 0.38f;

    menuPanel(ctx, panelX, panelY, panelW, panelH);
    menuLabelCentered(ctx, panelX + panelW * 0.5f, panelY + pad,
                      "Inventory", 2.0f * scale, {1.0f, 1.0f, 1.0f, 1.0f});
    menuLabelCentered(ctx, panelX + panelW * 0.5f, panelY + pad + titleH,
                      "Click to hotbar. E closes.",
                      1.2f * scale, {0.78f, 0.78f, 0.72f, 1.0f});

    const float gridX = panelX + pad;
    const float gridY = panelY + pad + titleH + hintH;

    for (int i = 0; i < Inventory::COUNT; ++i) {
        const int col = i % cols;
        const int row = i / cols;
        const float x = gridX + col * (cell + gap);
        const float y = gridY + row * (cell + gap);
        const BlockId id = Inventory::kItems[i];
        const bool hover = menuHit(ctx, x, y, cell, cell);
        hud.drawQuad(x, y, x + cell, y + cell, {0.18f, 0.18f, 0.18f, 0.90f});
        hud.drawQuad(x + 2.0f * scale, y + 2.0f * scale,
                     x + cell - 2.0f * scale, y + cell - 2.0f * scale,
                     hover ? glm::vec4{0.22f, 0.28f, 0.14f, 0.95f}
                           : glm::vec4{0.08f, 0.08f, 0.08f, 0.95f});
        drawBlockIcon(hud, x, y, cell, 7.0f * scale, id);
        if (hover && ctx.mouseReleased)
            hotbar.setSlot(hotbar.selectedIndex(), id);
    }

    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i) {
        const float x = g.barX + i * (g.slot + g.gap);
        if (menuHit(ctx, x, g.barY, g.slot, g.slot) && ctx.mouseReleased)
            hotbar.selectSlot(i);
    }

    Render(hud, hotbar, ctx.width, ctx.height);
}
