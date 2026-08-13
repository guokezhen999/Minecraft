//
// Created by 郭珂桢 on 2024/4/21.
//

#ifndef MINECRAFT_GAME_H
#define MINECRAFT_GAME_H

#include <glad/glad.h>

#include "Config.h"
#include "Camera.h"
#include "Renderer/RenderMaster.h"
#include "Renderer/OutlineRenderer.h"
#include "Renderer/CrosshairRenderer.h"
#include "Renderer/HotbarRenderer.h"
#include "Renderer/HudRenderer.h"
#include "Renderer/SkyRenderer.h"
#include "Physics/RayCast.h"
#include "Physics/Player.h"
#include "World/World.h"
#include "World/WorldSave.h"
#include "World/Block/BlockId.h"
#include "UI/Hotbar.h"
#include "UI/Menu.h"

#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

enum class GameScreen {
    Title,
    Worlds,
    CreateWorld,
    Settings,
    Playing,
    Paused
};

class Game
{
public:
    Game(Config &config, Camera &camera);
    ~Game();

    void setWindow(GLFWwindow* window);
    void Init();
    void shutdown();

    void Render(int windowWidth, int windowHeight);
    void Update(float deltaTime);
    void ProcessInput(float deltaTime);
    void endFrame();

    void OnLeftClick();
    void OnRightClick();
    void OnScroll(float yoffset);
    void OnEscape();
    void OnChar(unsigned int codepoint);
    void OnKey(int key, int action);
    void OnMouseButton(int button, int action);
    void setCursorPos(double x, double y);

    bool isPlaying() const { return m_screen == GameScreen::Playing; }
    GameScreen screen() const { return m_screen; }
    bool isCameraUnderwater() const { return m_cameraUnderwater; }
    glm::vec3 clearColor() const;

    Camera *m_Camera;
    RenderMaster m_RenderMaster;

    Config &m_Config;

    GLboolean Keys[1024];
    bool KeyProcessed[1024];
    bool IsPopState = false;

    std::unique_ptr<World> m_World;
    std::unique_ptr<Player> m_Player;

private:
    void updateTargetBlock();
    glm::vec3 horizontalLookDir() const;

    void setupTitleCamera();
    MenuContext makeMenuContext(int fbW, int fbH);
    void renderMenu(int fbW, int fbH);
    void drawTitle(MenuContext& ctx);
    void drawPaused(MenuContext& ctx);
    void drawWorlds(MenuContext& ctx);
    void drawCreateWorld(MenuContext& ctx);
    void drawSettings(MenuContext& ctx);

    void openWorlds(GameScreen back);
    void openSettings(GameScreen back);
    void openCreateWorld();
    void refreshWorldList();

    bool loadWorld(const std::string& folder);
    bool createWorld();
    void applyHeaderToRuntime();
    void captureHeaderFromRuntime();
    void flushWorld();
    void destroyWorld();
    void returnToTitle(bool save);
    void resumePlay();
    void quitGame();
    void deleteSelectedWorld();

    void applyFov();
    void applySensitivity();
    void applyVsync();
    void applyFullscreen();
    void persistSettings();

    std::unique_ptr<OutlineRenderer> m_outlineRenderer;
    std::unique_ptr<CrosshairRenderer> m_crosshairRenderer;
    std::unique_ptr<HotbarRenderer> m_hotbarRenderer;
    std::unique_ptr<HudRenderer> m_hudRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    Hotbar m_hotbar;
    RaycastHit m_target;
    bool m_cameraUnderwater = false;
    float m_tickAcc = 0.0f;

    GLFWwindow* m_window = nullptr;
    GameScreen m_screen = GameScreen::Title;
    GameScreen m_menuBack = GameScreen::Title;

    WorldSave::WorldHeader m_worldHeader;
    std::string m_worldFolder;
    std::vector<WorldSave::WorldInfo> m_worlds;
    int m_selectedWorld = -1;
    float m_worldScroll = 0.0f;
    bool m_confirmDelete = false;

    std::string m_newName = "New World";
    std::string m_newSeed;
    bool m_nameFocused = false;
    bool m_seedFocused = false;

    double m_cursorX = 0.0;
    double m_cursorY = 0.0;
    bool m_leftDown = false;
    bool m_leftPressed = false;
    bool m_leftReleased = false;
    char m_typedChars[16]{};
    int m_typedCount = 0;
    bool m_backspace = false;
    bool m_enter = false;
    float m_scrollDelta = 0.0f;
    bool m_ignoreMouseButtons = false;
};

#endif //MINECRAFT_GAME_H
