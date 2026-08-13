//
// Immediate-mode menu widgets
//

#include "Menu.h"
#include "../Renderer/HudRenderer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace {

const glm::vec4 kText{1.0f, 1.0f, 1.0f, 1.0f};
const glm::vec4 kTextDim{0.78f, 0.78f, 0.72f, 1.0f};
const glm::vec4 kPanel{0.16f, 0.16f, 0.18f, 0.92f};
const glm::vec4 kBtn{0.42f, 0.42f, 0.42f, 0.98f};
const glm::vec4 kBtnHover{0.55f, 0.68f, 0.32f, 0.98f};
const glm::vec4 kBtnPress{0.65f, 0.78f, 0.38f, 0.98f};
const glm::vec4 kBtnOff{0.22f, 0.22f, 0.22f, 0.90f};

float bodyScale(const MenuContext& ctx) {
    return std::max(1.0f, 2.0f * ctx.uiScale);
}

} // namespace

bool menuHit(const MenuContext& ctx, float x, float y, float w, float h) {
    return ctx.mouseX >= x && ctx.mouseX < x + w &&
           ctx.mouseY >= y && ctx.mouseY < y + h;
}

void menuPanel(MenuContext& ctx, float x, float y, float w, float h) {
    if (!ctx.hud)
        return;
    ctx.hud->drawQuad(x, y, x + w, y + h, kPanel);
    const float b = std::max(1.0f, ctx.uiScale);
    ctx.hud->drawQuad(x, y, x + w, y + b, {0.0f, 0.0f, 0.0f, 0.55f});
    ctx.hud->drawQuad(x, y + h - b, x + w, y + h, {0.0f, 0.0f, 0.0f, 0.55f});
    ctx.hud->drawQuad(x, y, x + b, y + h, {0.0f, 0.0f, 0.0f, 0.55f});
    ctx.hud->drawQuad(x + w - b, y, x + w, y + h, {0.0f, 0.0f, 0.0f, 0.55f});
}

void menuLabel(MenuContext& ctx, float x, float y, const std::string& text,
               float scale, const glm::vec4& color) {
    if (ctx.hud)
        ctx.hud->drawText(x, y, text, scale, color);
}

void menuLabelCentered(MenuContext& ctx, float cx, float y, const std::string& text,
                       float scale, const glm::vec4& color) {
    if (!ctx.hud)
        return;
    const float w = ctx.hud->textWidth(text, scale);
    ctx.hud->drawText(cx - w * 0.5f, y, text, scale, color);
}

bool menuButton(MenuContext& ctx, float x, float y, float w, float h,
                const std::string& label, bool enabled) {
    if (!ctx.hud)
        return false;

    const bool hovered = enabled && menuHit(ctx, x, y, w, h);
    glm::vec4 bg = kBtn;
    if (!enabled)
        bg = kBtnOff;
    else if (hovered && ctx.mouseDown)
        bg = kBtnPress;
    else if (hovered)
        bg = kBtnHover;

    ctx.hud->drawQuad(x, y, x + w, y + h, bg);
    const float b = std::max(1.0f, ctx.uiScale);
    const glm::vec4 edge = hovered && enabled
                               ? glm::vec4{0.85f, 0.85f, 0.80f, 0.90f}
                               : glm::vec4{0.08f, 0.08f, 0.08f, 0.90f};
    ctx.hud->drawQuad(x, y, x + w, y + b, edge);
    ctx.hud->drawQuad(x, y + h - b, x + w, y + h, edge);
    ctx.hud->drawQuad(x, y, x + b, y + h, edge);
    ctx.hud->drawQuad(x + w - b, y, x + w, y + h, edge);

    float scale = bodyScale(ctx);
    while (scale > 1.0f && ctx.hud->textWidth(label, scale) > w - 12.0f * ctx.uiScale)
        scale -= 0.25f;
    const float tw = ctx.hud->textWidth(label, scale);
    const float th = ctx.hud->textHeight(scale);
    ctx.hud->drawText(x + (w - tw) * 0.5f, y + (h - th) * 0.5f, label, scale,
                      enabled ? kText : kTextDim);

    return enabled && hovered && ctx.mouseReleased;
}

bool menuSlider(MenuContext& ctx, float x, float y, float w, float h,
                float vmin, float vmax, float& value, bool integer) {
    if (!ctx.hud || vmax <= vmin)
        return false;

    const bool hovered = menuHit(ctx, x, y, w, h);
    bool changed = false;
    if (hovered && ctx.mouseDown) {
        const float t = std::clamp((ctx.mouseX - x) / w, 0.0f, 1.0f);
        float next = vmin + t * (vmax - vmin);
        if (integer)
            next = std::round(next);
        if (next != value) {
            value = next;
            changed = true;
        }
    }

    value = std::clamp(value, vmin, vmax);
    if (integer)
        value = std::round(value);

    ctx.hud->drawQuad(x, y, x + w, y + h, {0.12f, 0.12f, 0.12f, 0.95f});
    const float t = (value - vmin) / (vmax - vmin);
    const float fill = w * t;
    ctx.hud->drawQuad(x, y, x + fill, y + h, hovered
                                                 ? glm::vec4{0.40f, 0.55f, 0.28f, 0.95f}
                                                 : glm::vec4{0.30f, 0.42f, 0.22f, 0.95f});
    const float knob = std::max(6.0f, 8.0f * ctx.uiScale);
    const float kx = x + fill - knob * 0.5f;
    ctx.hud->drawQuad(kx, y - 2.0f * ctx.uiScale, kx + knob, y + h + 2.0f * ctx.uiScale,
                      {0.90f, 0.90f, 0.88f, 1.0f});
    return changed;
}

bool menuToggle(MenuContext& ctx, float x, float y, float w, float h,
                const std::string& label, bool& value) {
    const std::string text = label + (value ? ": On" : ": Off");
    if (menuButton(ctx, x, y, w, h, text, true)) {
        value = !value;
        return true;
    }
    return false;
}

bool menuTextField(MenuContext& ctx, float x, float y, float w, float h,
                   std::string& text, bool& focused, int maxLen, bool seedMode) {
    if (!ctx.hud)
        return false;

    const bool hovered = menuHit(ctx, x, y, w, h);
    if (ctx.mousePressed) {
        focused = hovered;
    }

    bool changed = false;
    if (focused) {
        for (int i = 0; i < ctx.charCount; ++i) {
            const char c = ctx.chars[i];
            if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126)
                continue;
            if (static_cast<int>(text.size()) >= maxLen)
                break;
            if (seedMode) {
                if (c == '-' && text.empty()) {
                    text.push_back(c);
                    changed = true;
                } else if (c >= '0' && c <= '9') {
                    text.push_back(c);
                    changed = true;
                }
            } else {
                text.push_back(c);
                changed = true;
            }
        }
        if (ctx.backspace && !text.empty()) {
            text.pop_back();
            changed = true;
        }
    }

    const glm::vec4 bg = focused ? glm::vec4{0.08f, 0.08f, 0.10f, 0.95f}
                                 : glm::vec4{0.14f, 0.14f, 0.16f, 0.95f};
    ctx.hud->drawQuad(x, y, x + w, y + h, bg);
    const float b = std::max(1.0f, ctx.uiScale);
    const glm::vec4 edge = focused ? glm::vec4{0.85f, 0.85f, 0.55f, 1.0f}
                                   : glm::vec4{0.05f, 0.05f, 0.05f, 0.90f};
    ctx.hud->drawQuad(x, y, x + w, y + b, edge);
    ctx.hud->drawQuad(x, y + h - b, x + w, y + h, edge);
    ctx.hud->drawQuad(x, y, x + b, y + h, edge);
    ctx.hud->drawQuad(x + w - b, y, x + w, y + h, edge);

    float scale = bodyScale(ctx);
    std::string shown = text;
    if (focused)
        shown.push_back('_');
    while (scale > 1.0f && ctx.hud->textWidth(shown, scale) > w - 12.0f * ctx.uiScale)
        scale -= 0.25f;
    const float th = ctx.hud->textHeight(scale);
    ctx.hud->drawText(x + 8.0f * ctx.uiScale, y + (h - th) * 0.5f, shown, scale, kText);
    return changed;
}
