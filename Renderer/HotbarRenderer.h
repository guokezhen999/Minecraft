//
// Bottom-center hotbar HUD; inventory catalog overlay
//

#ifndef MINECRAFT_HOTBARRENDERER_H
#define MINECRAFT_HOTBARRENDERER_H

struct MenuContext;
class Hotbar;
class HudRenderer;

class HotbarRenderer {
public:
    void Render(HudRenderer& hud, const Hotbar& hotbar, int windowWidth, int windowHeight);
    void RenderInventory(HudRenderer& hud, Hotbar& hotbar, MenuContext& ctx);
};

#endif // MINECRAFT_HOTBARRENDERER_H
