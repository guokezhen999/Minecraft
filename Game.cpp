#include "Game.h"

#include "ResourceManager.h"
#include "World/World.h"
#include "World/WorldConstants.h"
#include "World/Block/BlockDataBase.h"
#include "World/Block/Water.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <sstream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {

const glm::vec4 kText{0.95f, 0.95f, 0.92f, 1.0f};
const glm::vec4 kTextDim{0.62f, 0.62f, 0.62f, 1.0f};
const glm::vec4 kWarn{0.92f, 0.55f, 0.35f, 1.0f};

float btnH(const MenuContext& ctx) { return 40.0f * ctx.uiScale; }
float gap(const MenuContext& ctx) { return 10.0f * ctx.uiScale; }

} // namespace

Game::Game(Config &config, Camera &m_Camera)
: m_Config(config), m_Camera(&m_Camera), Keys(), KeyProcessed()
{
}

Game::~Game()
{
    shutdown();
}

void Game::setWindow(GLFWwindow* window)
{
    m_window = window;
}

void Game::Init()
{
    BlockDatabase::Get();
    WorldSave::ensureSavesDirs();

    ResourceManager::BlockShader = new BasicShader("resources/shaders/basic.vert",
                                               "resources/shaders/basic.frag");
    ResourceManager::BlockShader->Use();
    ResourceManager::BlockShader->SetInteger("texture1", 0);
    ResourceManager::BlockTexture = new BasicTexture("resources/textures/defaultPack.png");

    m_outlineRenderer = std::make_unique<OutlineRenderer>();
    m_crosshairRenderer = std::make_unique<CrosshairRenderer>();
    m_hotbarRenderer = std::make_unique<HotbarRenderer>();
    m_hudRenderer = std::make_unique<HudRenderer>();
    m_skyRenderer = std::make_unique<SkyRenderer>();

    m_Camera->Zoom = static_cast<float>(m_Config.fov);
    m_Camera->MouseSensitivity = m_Config.mouseSensitivity;
    m_Camera->updateMatrices();
    setupTitleCamera();
    m_screen = GameScreen::Title;
}

void Game::shutdown()
{
    flushWorld();
    destroyWorld();
    m_RenderMaster.shutdown();
    m_outlineRenderer.reset();
    m_crosshairRenderer.reset();
    m_hotbarRenderer.reset();
    m_hudRenderer.reset();
    m_skyRenderer.reset();

    delete ResourceManager::BlockShader;
    ResourceManager::BlockShader = nullptr;
    delete ResourceManager::BlockTexture;
    ResourceManager::BlockTexture = nullptr;
}

void Game::setupTitleCamera()
{
    m_Camera->SetPosition(glm::vec3(0.0f, 78.0f, 0.0f));
    m_Camera->SetLook(-20.0f, 8.0f);
}

void Game::Render(int windowWidth, int windowHeight)
{
    Atmosphere atmo;
    if (m_World)
        atmo = m_World->getAtmosphere();

    if (m_skyRenderer)
        m_skyRenderer->Render(*m_Camera, m_cameraUnderwater && m_World != nullptr, atmo);

    if (m_World)
        m_World->Render(m_RenderMaster, *m_Camera, m_cameraUnderwater);

    if (m_screen == GameScreen::Playing) {
        if (m_target.hit && m_outlineRenderer)
            m_outlineRenderer->Render(*m_Camera, m_target.blockPos);
        if (m_crosshairRenderer)
            m_crosshairRenderer->Render(windowWidth, windowHeight);
        if (m_hudRenderer && m_hotbarRenderer) {
            m_hudRenderer->begin(windowWidth, windowHeight);
            m_hotbarRenderer->Render(*m_hudRenderer, m_hotbar, windowWidth, windowHeight);
            m_hudRenderer->end();
        }
        return;
    }

    if (m_hudRenderer) {
        m_hudRenderer->begin(windowWidth, windowHeight);
        if (m_World && (m_screen == GameScreen::Paused ||
                        m_menuBack == GameScreen::Paused)) {
            m_hudRenderer->drawQuad(0.0f, 0.0f,
                                    static_cast<float>(windowWidth),
                                    static_cast<float>(windowHeight),
                                    {0.0f, 0.0f, 0.0f, 0.55f});
        }
        renderMenu(windowWidth, windowHeight);
        m_hudRenderer->end();
    }
}

void Game::Update(float deltaTime)
{
    if (m_screen != GameScreen::Playing || !m_World || !m_Player)
        return;

    m_World->setRenderDistance(m_Config.renderDistance);
    m_World->Update(m_Camera->Position);

    // Spawn column is still generating: skip physics so "unloaded = solid"
    // does not eject the player out the top of the world.
    const glm::vec3 feet = m_Player->getPosition();
    const int pcx = World::worldToChunk(static_cast<int>(std::floor(feet.x)));
    const int pcz = World::worldToChunk(static_cast<int>(std::floor(feet.z)));
    if (!m_World->isChunkLoaded(pcx, pcz)) {
        m_Player->syncCamera(*m_Camera);
        return;
    }

    m_Player->update(*m_World, deltaTime);
    m_Player->syncCamera(*m_Camera);

    m_cameraUnderwater = m_World->isCameraUnderwater(m_Camera->Position);

    m_tickAcc += deltaTime * static_cast<float>(TICKS_PER_SECOND) * WORLD_TIME_SCALE;
    if (Keys[GLFW_KEY_T]) {
        m_tickAcc += deltaTime * static_cast<float>(TICKS_PER_SECOND) *
                     WORLD_TIME_SCALE * (TIME_FAST_FORWARD - 1.0f);
    }
    int ticks = 0;
    while (m_tickAcc >= 1.0f) {
        m_tickAcc -= 1.0f;
        ++ticks;
    }
    if (ticks > 0)
        m_World->advanceTime(ticks);

    updateTargetBlock();
}

glm::vec3 Game::clearColor() const
{
    if (m_cameraUnderwater && m_World)
        return {UNDERWATER_FOG_R, UNDERWATER_FOG_G, UNDERWATER_FOG_B};
    if (m_World)
        return m_World->getAtmosphere().fogColor;
    return {FOG_R, FOG_G, FOG_B};
}

glm::vec3 Game::horizontalLookDir() const
{
    const float yaw = glm::radians(m_Camera->Yaw);
    return glm::normalize(glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw)));
}

void Game::updateTargetBlock()
{
    if (!m_World)
        return;
    m_target = raycastWorld(*m_World, m_Camera->Position, m_Camera->Front,
                            RAYCAST_REACH, /*hitFluids=*/false);
    if (!m_target.hit) {
        m_target = raycastWorld(*m_World, m_Camera->Position, m_Camera->Front,
                                RAYCAST_REACH, /*hitFluids=*/true);
    }
}

void Game::OnLeftClick()
{
    if (m_ignoreMouseButtons || m_screen != GameScreen::Playing || !m_target.hit || !m_World)
        return;
    m_World->setBlock(m_target.blockPos.x, m_target.blockPos.y, m_target.blockPos.z,
                      ChunkBlock(BlockId::Air));
    updateTargetBlock();
}

void Game::OnRightClick()
{
    if (m_ignoreMouseButtons || m_screen != GameScreen::Playing || !m_target.hit || !m_World || !m_Player)
        return;

    const BlockId selected = m_hotbar.selectedBlock();
    ChunkBlock toPlace(selected);
    if (selected == BlockId::Water)
        toPlace = Water::makeSource();

    const glm::ivec3& hit = m_target.blockPos;
    const ChunkBlock targeted = m_World->getBlock(hit.x, hit.y, hit.z);

    if (selected != BlockId::Water && Water::isReplaceable(targeted)) {
        if (!m_Player->intersectsBlock(hit.x, hit.y, hit.z)) {
            m_World->setBlock(hit.x, hit.y, hit.z, toPlace);
            updateTargetBlock();
        }
        return;
    }

    const glm::ivec3& p = m_target.previousPos;
    if (p.y < 0 || p.y >= WORLD_HEIGHT)
        return;

    if (!Water::isReplaceable(m_World->getBlock(p.x, p.y, p.z)))
        return;

    if (m_Player->intersectsBlock(p.x, p.y, p.z))
        return;

    m_World->setBlock(p.x, p.y, p.z, toPlace);
    updateTargetBlock();
}

void Game::OnScroll(float yoffset)
{
    if (m_screen == GameScreen::Playing) {
        if (yoffset > 0.0f)
            m_hotbar.cycleSlot(-1);
        else if (yoffset < 0.0f)
            m_hotbar.cycleSlot(1);
        return;
    }
    m_scrollDelta += yoffset;
}

void Game::OnChar(unsigned int codepoint)
{
    if (m_screen == GameScreen::Playing)
        return;
    if (codepoint < 32 || codepoint > 126)
        return;
    if (m_typedCount < 16)
        m_typedChars[m_typedCount++] = static_cast<char>(codepoint);
}

void Game::OnKey(int key, int action)
{
    if (m_screen == GameScreen::Playing)
        return;
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    if (key == GLFW_KEY_BACKSPACE)
        m_backspace = true;
    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
        m_enter = true;
}

void Game::OnMouseButton(int button, int action)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;
    if (m_ignoreMouseButtons) {
        if (action == GLFW_RELEASE)
            m_ignoreMouseButtons = false;
        return;
    }
    if (action == GLFW_PRESS) {
        m_leftDown = true;
        m_leftPressed = true;
    } else if (action == GLFW_RELEASE) {
        m_leftDown = false;
        m_leftReleased = true;
    }
}

void Game::setCursorPos(double x, double y)
{
    m_cursorX = x;
    m_cursorY = y;
}

void Game::endFrame()
{
    m_leftPressed = false;
    m_leftReleased = false;
    m_typedCount = 0;
    m_backspace = false;
    m_enter = false;
    m_scrollDelta = 0.0f;
    if (m_screen == GameScreen::Playing)
        m_ignoreMouseButtons = false;
}

void Game::OnEscape()
{
    switch (m_screen) {
    case GameScreen::Playing:
        m_screen = GameScreen::Paused;
        break;
    case GameScreen::Paused:
        resumePlay();
        break;
    case GameScreen::CreateWorld:
        m_screen = GameScreen::Worlds;
        m_nameFocused = false;
        m_seedFocused = false;
        break;
    case GameScreen::Worlds:
    case GameScreen::Settings:
        persistSettings();
        m_screen = m_menuBack;
        m_confirmDelete = false;
        if (m_screen == GameScreen::Title)
            setupTitleCamera();
        break;
    case GameScreen::Title:
        break;
    }
}

void Game::ProcessInput(float deltaTime)
{
    if (m_screen != GameScreen::Playing || !m_Player)
        return;

    if (Keys[GLFW_KEY_V] && !KeyProcessed[GLFW_KEY_V]) {
        m_Player->toggleFlying();
        KeyProcessed[GLFW_KEY_V] = true;
    }
    if (!Keys[GLFW_KEY_V])
        KeyProcessed[GLFW_KEY_V] = false;

    const bool sneaking = Keys[GLFW_KEY_LEFT_SHIFT] || Keys[GLFW_KEY_RIGHT_SHIFT];
    m_Player->setSneaking(sneaking && !m_Player->isFlying());

    const glm::vec3 forward = horizontalLookDir();
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 wish(0.0f);
    if (Keys[GLFW_KEY_W]) wish += forward;
    if (Keys[GLFW_KEY_S]) wish -= forward;
    if (Keys[GLFW_KEY_A]) wish -= right;
    if (Keys[GLFW_KEY_D]) wish += right;

    if (m_Player->isFlying()) {
        if (Keys[GLFW_KEY_SPACE] || Keys[GLFW_KEY_R])
            wish.y += 1.0f;
        if (Keys[GLFW_KEY_LEFT_SHIFT] || Keys[GLFW_KEY_F])
            wish.y -= 1.0f;
    }

    m_Player->setMoveInput(wish);
    m_Player->setJumpPressed(Keys[GLFW_KEY_SPACE] && !m_Player->isFlying());

    const float ARROW_LOOK_SPEED = 120.0f;
    float lookDelta = ARROW_LOOK_SPEED * deltaTime;
    if (Keys[GLFW_KEY_UP])
        m_Camera->ProcessMouseMovement(0.0f,  lookDelta);
    if (Keys[GLFW_KEY_DOWN])
        m_Camera->ProcessMouseMovement(0.0f, -lookDelta);
    if (Keys[GLFW_KEY_LEFT])
        m_Camera->ProcessMouseMovement(-lookDelta, 0.0f);
    if (Keys[GLFW_KEY_RIGHT])
        m_Camera->ProcessMouseMovement( lookDelta, 0.0f);

    static const int slotKeys[Hotbar::SLOT_COUNT] = {
        GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3,
        GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6,
        GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
    };
    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i) {
        const int key = slotKeys[i];
        if (Keys[key] && !KeyProcessed[key]) {
            m_hotbar.selectSlot(i);
            KeyProcessed[key] = true;
        }
        if (!Keys[key])
            KeyProcessed[key] = false;
    }
}

MenuContext Game::makeMenuContext(int fbW, int fbH)
{
    MenuContext ctx;
    ctx.hud = m_hudRenderer.get();
    ctx.width = fbW;
    ctx.height = fbH;
    ctx.uiScale = std::max(1.0f, std::min(fbW, fbH) / 720.0f);

    int winW = fbW, winH = fbH;
    if (m_window) {
        glfwGetWindowSize(m_window, &winW, &winH);
        if (winW <= 0) winW = fbW;
        if (winH <= 0) winH = fbH;
    }
    ctx.mouseX = static_cast<float>(m_cursorX) * static_cast<float>(fbW) / static_cast<float>(winW);
    ctx.mouseY = static_cast<float>(m_cursorY) * static_cast<float>(fbH) / static_cast<float>(winH);
    ctx.mouseDown = m_leftDown;
    ctx.mousePressed = m_leftPressed;
    ctx.mouseReleased = m_leftReleased;
    ctx.charCount = m_typedCount;
    for (int i = 0; i < m_typedCount; ++i)
        ctx.chars[i] = m_typedChars[i];
    ctx.backspace = m_backspace;
    ctx.enter = m_enter;
    ctx.scroll = m_scrollDelta;
    return ctx;
}

void Game::renderMenu(int fbW, int fbH)
{
    MenuContext ctx = makeMenuContext(fbW, fbH);
    switch (m_screen) {
    case GameScreen::Title:       drawTitle(ctx); break;
    case GameScreen::Paused:      drawPaused(ctx); break;
    case GameScreen::Worlds:      drawWorlds(ctx); break;
    case GameScreen::CreateWorld: drawCreateWorld(ctx); break;
    case GameScreen::Settings:    drawSettings(ctx); break;
    default: break;
    }
}

void Game::openWorlds(GameScreen back)
{
    m_menuBack = back;
    m_confirmDelete = false;
    refreshWorldList();
    m_screen = GameScreen::Worlds;
}

void Game::openSettings(GameScreen back)
{
    m_menuBack = back;
    m_screen = GameScreen::Settings;
}

void Game::openCreateWorld()
{
    m_newName = "New World";
    m_newSeed.clear();
    m_nameFocused = false;
    m_seedFocused = false;
    m_screen = GameScreen::CreateWorld;
}

void Game::refreshWorldList()
{
    m_worlds = WorldSave::listWorlds();
    if (m_selectedWorld >= static_cast<int>(m_worlds.size()))
        m_selectedWorld = static_cast<int>(m_worlds.size()) - 1;
}

void Game::resumePlay()
{
    if (!m_World)
        return;
    m_screen = GameScreen::Playing;
    m_ignoreMouseButtons = true;
    m_target.hit = false;
}

void Game::quitGame()
{
    flushWorld();
    if (m_window)
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Game::returnToTitle(bool save)
{
    if (save)
        flushWorld();
    destroyWorld();
    m_screen = GameScreen::Title;
    m_menuBack = GameScreen::Title;
    setupTitleCamera();
    m_cameraUnderwater = false;
}

void Game::destroyWorld()
{
    m_World.reset();
    m_Player.reset();
    m_worldFolder.clear();
    m_target.hit = false;
}

void Game::captureHeaderFromRuntime()
{
    if (m_Player) {
        const glm::vec3 p = m_Player->getPosition();
        m_worldHeader.playerX = p.x;
        m_worldHeader.playerY = p.y;
        m_worldHeader.playerZ = p.z;
        m_worldHeader.flying = m_Player->isFlying();
    }
    if (m_Camera) {
        m_worldHeader.yaw = m_Camera->Yaw;
        m_worldHeader.pitch = m_Camera->Pitch;
    }
    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i)
        m_worldHeader.hotbar[i] = static_cast<int>(m_hotbar.slot(i));
    m_worldHeader.selected = m_hotbar.selectedIndex();
    if (m_World)
        m_worldHeader.gameTime = m_World->getWorldTick();
}

void Game::applyHeaderToRuntime()
{
    m_Player = std::make_unique<Player>();
    m_Player->setPosition(glm::vec3(m_worldHeader.playerX, m_worldHeader.playerY,
                                    m_worldHeader.playerZ));
    m_Player->setFlying(m_worldHeader.flying);
    m_Camera->SetLook(m_worldHeader.yaw, m_worldHeader.pitch);
    m_Player->syncCamera(*m_Camera);
    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i)
        m_hotbar.setSlot(i, static_cast<BlockId>(m_worldHeader.hotbar[i]));
    m_hotbar.selectSlot(m_worldHeader.selected);
    if (m_World)
        m_World->setWorldTick(m_worldHeader.gameTime);
    m_cameraUnderwater = false;
    m_tickAcc = 0.0f;
    m_target.hit = false;
}

void Game::flushWorld()
{
    if (!m_World || m_worldFolder.empty())
        return;
    m_World->flushDirtyColumns();
    captureHeaderFromRuntime();
    m_worldHeader.lastPlayed = static_cast<int64_t>(std::time(nullptr));
    WorldSave::writeWorldDat(m_worldFolder, m_worldHeader);
}

bool Game::loadWorld(const std::string& folder)
{
    WorldSave::WorldHeader header;
    if (!WorldSave::readWorldDat(folder, header))
        return false;

    if (m_World && m_worldFolder == folder) {
        resumePlay();
        return true;
    }

    if (m_World)
        flushWorld();
    destroyWorld();

    m_worldFolder = folder;
    m_worldHeader = header;
    m_World = std::make_unique<World>(header.seed, WorldSave::worldDir(folder),
                                      m_Config.renderDistance);
    applyHeaderToRuntime();
    m_Config.lastWorld = folder;
    persistSettings();
    m_World->Update(m_Camera->Position);
    resumePlay();
    return true;
}

bool Game::createWorld()
{
    int32_t seed = 0;
    if (m_newSeed.empty()) {
        seed = WorldSave::randomSeed();
    } else if (!WorldSave::parseSeed(m_newSeed, seed)) {
        return false;
    }

    std::string name = m_newName;
    if (name.empty())
        name = "New World";
    const std::string folder = WorldSave::uniqueFolder(WorldSave::slugify(name));

    if (m_World)
        flushWorld();
    destroyWorld();

    const auto now = static_cast<int64_t>(std::time(nullptr));
    WorldSave::WorldHeader header;
    header.name = name;
    header.seed = seed;
    header.created = now;
    header.lastPlayed = now;

    m_World = std::make_unique<World>(seed, WorldSave::worldDir(folder),
                                      m_Config.renderDistance);
    const int surfaceY = m_World->getSurfaceHeight(0, 0);
    header.playerX = 0.5f;
    header.playerY = static_cast<float>(std::max(surfaceY, WATER_LEVEL)) + 2.0f;
    header.playerZ = 0.5f;

    if (!WorldSave::writeWorldDat(folder, header)) {
        destroyWorld();
        return false;
    }

    m_worldFolder = folder;
    m_worldHeader = header;
    applyHeaderToRuntime();
    m_Config.lastWorld = folder;
    persistSettings();
    m_World->Update(m_Camera->Position);
    resumePlay();
    return true;
}

void Game::deleteSelectedWorld()
{
    if (m_selectedWorld < 0 || m_selectedWorld >= static_cast<int>(m_worlds.size()))
        return;
    const std::string folder = m_worlds[m_selectedWorld].folder;
    const bool current = m_World && m_worldFolder == folder;
    if (current)
        destroyWorld();
    WorldSave::deleteWorld(folder);
    if (m_Config.lastWorld == folder) {
        m_Config.lastWorld.clear();
        persistSettings();
    }
    if (current) {
        m_screen = GameScreen::Title;
        m_menuBack = GameScreen::Title;
        setupTitleCamera();
        m_cameraUnderwater = false;
    }
    m_confirmDelete = false;
    refreshWorldList();
}

void Game::applyFov()
{
    m_Camera->Zoom = static_cast<float>(m_Config.fov);
    m_Camera->updateMatrices();
}

void Game::applySensitivity()
{
    m_Camera->MouseSensitivity = m_Config.mouseSensitivity;
}

void Game::applyVsync()
{
    if (m_window)
        glfwSwapInterval(m_Config.vsync ? 1 : 0);
}

void Game::applyFullscreen()
{
    if (!m_window)
        return;
    if (m_Config.isFullscreen) {
        glfwGetWindowPos(m_window, &m_Config.windowPosX, &m_Config.windowPosY);
        glfwGetWindowSize(m_window, &m_Config.windowX, &m_Config.windowY);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
            return;
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
            return;
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_window, nullptr,
                             m_Config.windowPosX, m_Config.windowPosY,
                             m_Config.windowX, m_Config.windowY, 0);
    }
    applyVsync();
}

void Game::persistSettings()
{
    WorldSave::saveSettings(m_Config);
}

void Game::drawTitle(MenuContext& ctx)
{
    const float s = ctx.uiScale;
    const float panelW = 360.0f * s;
    const float bh = btnH(ctx);
    const float g = gap(ctx);
    const float panelH = 64.0f * s + 4.0f * (bh + g) + 24.0f * s;
    const float px = (ctx.width - panelW) * 0.5f;
    const float py = (ctx.height - panelH) * 0.5f;

    menuPanel(ctx, px, py, panelW, panelH);
    menuLabelCentered(ctx, ctx.width * 0.5f, py + 18.0f * s, "MINECRAFT", 4.0f * s, kText);

    float y = py + 64.0f * s;
    const float bx = px + 30.0f * s;
    const float bw = panelW - 60.0f * s;

    if (menuButton(ctx, bx, y, bw, bh, "Continue")) {
        if (!m_Config.lastWorld.empty() && WorldSave::worldExists(m_Config.lastWorld) &&
            loadWorld(m_Config.lastWorld)) {
            return;
        }
        openWorlds(GameScreen::Title);
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Worlds")) {
        openWorlds(GameScreen::Title);
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Settings")) {
        openSettings(GameScreen::Title);
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Quit Game"))
        quitGame();
}

void Game::drawPaused(MenuContext& ctx)
{
    const float s = ctx.uiScale;
    const float panelW = 360.0f * s;
    const float bh = btnH(ctx);
    const float g = gap(ctx);
    const float panelH = 56.0f * s + 5.0f * (bh + g) + 16.0f * s;
    const float px = (ctx.width - panelW) * 0.5f;
    const float py = (ctx.height - panelH) * 0.5f;

    menuPanel(ctx, px, py, panelW, panelH);
    menuLabelCentered(ctx, ctx.width * 0.5f, py + 16.0f * s, "Paused", 3.0f * s, kText);

    float y = py + 52.0f * s;
    const float bx = px + 30.0f * s;
    const float bw = panelW - 60.0f * s;

    if (menuButton(ctx, bx, y, bw, bh, "Resume")) {
        resumePlay();
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Worlds")) {
        openWorlds(GameScreen::Paused);
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Settings")) {
        openSettings(GameScreen::Paused);
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Save & Title")) {
        returnToTitle(true);
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Quit Game"))
        quitGame();
}

void Game::drawWorlds(MenuContext& ctx)
{
    const float s = ctx.uiScale;
    const float panelW = 560.0f * s;
    const float bh = btnH(ctx);
    const float g = gap(ctx);
    const float rowH = 52.0f * s;
    const int visible = 6;
    const float listH = visible * rowH;
    const float panelH = 56.0f * s + listH + 4.0f * (bh + g) + 48.0f * s;
    const float px = (ctx.width - panelW) * 0.5f;
    const float py = (ctx.height - panelH) * 0.5f;

    menuPanel(ctx, px, py, panelW, panelH);
    menuLabelCentered(ctx, ctx.width * 0.5f, py + 16.0f * s, "Worlds", 3.0f * s, kText);

    const float listX = px + 20.0f * s;
    const float listY = py + 52.0f * s;
    const float listW = panelW - 40.0f * s;
    ctx.hud->drawQuad(listX, listY, listX + listW, listY + listH,
                      {0.02f, 0.02f, 0.03f, 0.55f});

    const int n = static_cast<int>(m_worlds.size());
    const int maxScroll = std::max(0, n - visible);
    if (menuHit(ctx, listX, listY, listW, listH) && ctx.scroll != 0.0f) {
        m_worldScroll -= ctx.scroll;
    }
    m_worldScroll = std::clamp(m_worldScroll, 0.0f, static_cast<float>(maxScroll));
    const int start = static_cast<int>(m_worldScroll);

    if (n == 0) {
        menuLabelCentered(ctx, ctx.width * 0.5f, listY + listH * 0.5f - 8.0f * s,
                          "No worlds yet", 2.0f * s, kTextDim);
    }

    for (int i = 0; i < visible; ++i) {
        const int idx = start + i;
        if (idx >= n)
            break;
        const float ry = listY + i * rowH;
        const bool selected = (idx == m_selectedWorld);
        const bool hovered = menuHit(ctx, listX, ry, listW, rowH);
        glm::vec4 bg{0.16f, 0.16f, 0.18f, selected ? 0.95f : 0.55f};
        if (hovered)
            bg = {0.28f, 0.32f, 0.22f, 0.90f};
        ctx.hud->drawQuad(listX, ry, listX + listW, ry + rowH - 2.0f * s, bg);

        const auto& info = m_worlds[idx];
        if (hovered && ctx.mouseReleased) {
            m_selectedWorld = idx;
            m_confirmDelete = false;
        }

        const float ts = 2.0f * s;
        if (info.corrupt) {
            menuLabel(ctx, listX + 10.0f * s, ry + 8.0f * s, "Unreadable", ts, kTextDim);
            menuLabel(ctx, listX + 10.0f * s, ry + 26.0f * s, info.folder, 1.5f * s, kTextDim);
        } else {
            std::ostringstream line2;
            line2 << "Seed " << info.header.seed << "   "
                  << WorldSave::formatTime(info.header.lastPlayed);
            menuLabel(ctx, listX + 10.0f * s, ry + 6.0f * s, info.header.name, ts, kText);
            menuLabel(ctx, listX + 10.0f * s, ry + 28.0f * s, line2.str(), 1.5f * s, kTextDim);
        }
    }

    float y = listY + listH + 12.0f * s;
    const float bx = px + 20.0f * s;
    const float half = (listW - g) * 0.5f;
    const bool hasSel = m_selectedWorld >= 0 && m_selectedWorld < n;
    const bool canPlay = hasSel && !m_worlds[m_selectedWorld].corrupt;

    if (menuButton(ctx, bx, y, half, bh, "Play", canPlay)) {
        loadWorld(m_worlds[m_selectedWorld].folder);
        return;
    }
    if (menuButton(ctx, bx + half + g, y, half, bh, "Delete", hasSel)) {
        m_confirmDelete = true;
    }

    y += bh + g;
    if (m_confirmDelete && hasSel) {
        menuLabelCentered(ctx, ctx.width * 0.5f, y - 2.0f * s,
                          "Delete cannot be undone", 1.5f * s, kWarn);
        if (menuButton(ctx, bx, y + 16.0f * s, half, bh, "Confirm")) {
            deleteSelectedWorld();
            return;
        }
        if (menuButton(ctx, bx + half + g, y + 16.0f * s, half, bh, "Cancel"))
            m_confirmDelete = false;
        y += bh + g + 16.0f * s;
    }

    if (menuButton(ctx, bx, y, listW, bh, "Create World")) {
        openCreateWorld();
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, listW, bh, "Back")) {
        m_confirmDelete = false;
        m_screen = m_menuBack;
    }
}

void Game::drawCreateWorld(MenuContext& ctx)
{
    const float s = ctx.uiScale;
    const float panelW = 480.0f * s;
    const float bh = btnH(ctx);
    const float g = gap(ctx);
    const float fieldH = 36.0f * s;
    const float panelH = 64.0f * s + 2.0f * (22.0f * s + fieldH + g) + bh + g + bh + 24.0f * s;
    const float px = (ctx.width - panelW) * 0.5f;
    const float py = (ctx.height - panelH) * 0.5f;

    menuPanel(ctx, px, py, panelW, panelH);
    menuLabelCentered(ctx, ctx.width * 0.5f, py + 16.0f * s, "Create World", 3.0f * s, kText);

    const float bx = px + 24.0f * s;
    const float bw = panelW - 48.0f * s;
    float y = py + 60.0f * s;

    menuLabel(ctx, bx, y, "Name", 2.0f * s, kTextDim);
    y += 20.0f * s;
    menuTextField(ctx, bx, y, bw, fieldH, m_newName, m_nameFocused, 24, false);
    y += fieldH + g;

    menuLabel(ctx, bx, y, "Seed (empty = random)", 2.0f * s, kTextDim);
    y += 20.0f * s;
    const float randW = 110.0f * s;
    menuTextField(ctx, bx, y, bw - randW - g, fieldH, m_newSeed, m_seedFocused, 12, true);
    if (menuButton(ctx, bx + bw - randW, y, randW, fieldH, "Random")) {
        m_newSeed = std::to_string(WorldSave::randomSeed());
        m_seedFocused = false;
    }
    y += fieldH + g * 1.5f;

    int32_t parsed = 0;
    const bool seedOk = m_newSeed.empty() || WorldSave::parseSeed(m_newSeed, parsed);
    if (!seedOk)
        menuLabel(ctx, bx, y - 8.0f * s, "Seed must be an integer", 1.5f * s, kWarn);

    if (menuButton(ctx, bx, y, bw, bh, "Create", seedOk) || (ctx.enter && seedOk)) {
        createWorld();
        return;
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Back")) {
        m_nameFocused = false;
        m_seedFocused = false;
        m_screen = GameScreen::Worlds;
    }
}

void Game::drawSettings(MenuContext& ctx)
{
    const float s = ctx.uiScale;
    const float panelW = 480.0f * s;
    const float bh = btnH(ctx);
    const float g = gap(ctx);
    const float rowH = 28.0f * s;
    const float panelH = 56.0f * s + 5.0f * (22.0f * s + rowH + g) + bh + 28.0f * s;
    const float px = (ctx.width - panelW) * 0.5f;
    const float py = (ctx.height - panelH) * 0.5f;

    menuPanel(ctx, px, py, panelW, panelH);
    menuLabelCentered(ctx, ctx.width * 0.5f, py + 16.0f * s, "Settings", 3.0f * s, kText);

    const float bx = px + 24.0f * s;
    const float bw = panelW - 48.0f * s;
    float y = py + 56.0f * s;
    const float ts = 2.0f * s;

    {
        float fov = static_cast<float>(m_Config.fov);
        menuLabel(ctx, bx, y, "FOV  " + std::to_string(m_Config.fov), ts, kText);
        y += 20.0f * s;
        if (menuSlider(ctx, bx, y, bw, rowH, 60.0f, 110.0f, fov, true)) {
            m_Config.fov = static_cast<int>(fov);
            applyFov();
            persistSettings();
        }
        y += rowH + g;
    }
    {
        float rd = static_cast<float>(m_Config.renderDistance);
        menuLabel(ctx, bx, y,
                  "Render Distance  " + std::to_string(m_Config.renderDistance) +
                      (m_World ? "  (after Resume)" : ""),
                  ts, kText);
        y += 20.0f * s;
        if (menuSlider(ctx, bx, y, bw, rowH, 4.0f, 16.0f, rd, true)) {
            m_Config.renderDistance = static_cast<int>(rd);
            if (m_World)
                m_World->setRenderDistance(m_Config.renderDistance);
            persistSettings();
        }
        y += rowH + g;
    }
    {
        float sens = m_Config.mouseSensitivity;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Mouse Sensitivity  %.2f", sens);
        menuLabel(ctx, bx, y, buf, ts, kText);
        y += 20.0f * s;
        if (menuSlider(ctx, bx, y, bw, rowH, 0.04f, 0.30f, sens, false)) {
            m_Config.mouseSensitivity = sens;
            applySensitivity();
            persistSettings();
        }
        y += rowH + g;
    }

    if (menuToggle(ctx, bx, y, bw, bh, "Vsync", m_Config.vsync)) {
        applyVsync();
        persistSettings();
    }
    y += bh + g;
    if (menuToggle(ctx, bx, y, bw, bh, "Fullscreen", m_Config.isFullscreen)) {
        applyFullscreen();
        persistSettings();
    }
    y += bh + g;
    if (menuButton(ctx, bx, y, bw, bh, "Back")) {
        persistSettings();
        m_screen = m_menuBack;
        if (m_screen == GameScreen::Title)
            setupTitleCamera();
    }
}
