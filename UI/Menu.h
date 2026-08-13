//
// Immediate-mode menu widgets (mouse + ASCII text). Drawn through HudRenderer.
//

#ifndef MINECRAFT_MENU_H
#define MINECRAFT_MENU_H

#include <glm/glm.hpp>

#include <string>

class HudRenderer;

struct MenuContext {
    HudRenderer* hud = nullptr;
    int width = 0;
    int height = 0;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseDown = false;
    bool mousePressed = false;
    bool mouseReleased = false;
    float uiScale = 1.0f;
    char chars[16]{};
    int charCount = 0;
    bool backspace = false;
    bool enter = false;
    float scroll = 0.0f;
};

bool menuHit(const MenuContext& ctx, float x, float y, float w, float h);

void menuPanel(MenuContext& ctx, float x, float y, float w, float h);
void menuLabel(MenuContext& ctx, float x, float y, const std::string& text,
               float scale, const glm::vec4& color);
void menuLabelCentered(MenuContext& ctx, float cx, float y, const std::string& text,
                       float scale, const glm::vec4& color);

bool menuButton(MenuContext& ctx, float x, float y, float w, float h,
                const std::string& label, bool enabled = true);

bool menuSlider(MenuContext& ctx, float x, float y, float w, float h,
                float vmin, float vmax, float& value, bool integer = false);

bool menuToggle(MenuContext& ctx, float x, float y, float w, float h,
                const std::string& label, bool& value);

bool menuTextField(MenuContext& ctx, float x, float y, float w, float h,
                   std::string& text, bool& focused, int maxLen, bool seedMode);

#endif // MINECRAFT_MENU_H
