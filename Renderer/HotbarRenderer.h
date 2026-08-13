//
// Bottom-center hotbar HUD (slot frames + block icons + selection)
//

#ifndef MINECRAFT_HOTBARRENDERER_H
#define MINECRAFT_HOTBARRENDERER_H

class Hotbar;
class HudRenderer;

class HotbarRenderer {
public:
    void Render(HudRenderer& hud, const Hotbar& hotbar, int windowWidth, int windowHeight);
};

#endif // MINECRAFT_HOTBARRENDERER_H
