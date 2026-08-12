#include "Game.h"

#include "Renderer/CubeRenderer.h"
#include "ResourceManager.h"
#include "World/World.h"
#include "World/WorldConstants.h"
#include "World/Block/BlockDataBase.h"

#include <cmath>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

Game::Game(Config &config, Camera &m_Camera)
: m_Config(config), m_Camera(&m_Camera), Keys(), KeyProcessed()
{
}

void Game::Init()
{
    BlockDatabase::Get();

    ResourceManager::BlockShader = new BasicShader("resources/shaders/basic.vert",
                                               "resources/shaders/basic.frag");
    ResourceManager::BlockShader->Use();
    ResourceManager::BlockShader->SetInteger("texture1", 0);
    ResourceManager::BlockTexture = new BasicTexture("resources/textures/defaultPack.png");

    m_RenderMaster.InitCubeRenderer();
    m_outlineRenderer = std::make_unique<OutlineRenderer>();
    m_crosshairRenderer = std::make_unique<CrosshairRenderer>();

    constexpr int WORLD_SEED = 114514;
    m_World = std::make_unique<World>(WORLD_SEED);

    m_Player = std::make_unique<Player>();
    // Spawn just above terrain (and above sea level if the column is flooded)
    constexpr int SPAWN_X = 0;
    constexpr int SPAWN_Z = 0;
    const int surfaceY = m_World->getSurfaceHeight(SPAWN_X, SPAWN_Z);
    const float spawnY =
        static_cast<float>(std::max(surfaceY, WATER_LEVEL)) + 2.0f;
    m_Player->setPosition(glm::vec3(static_cast<float>(SPAWN_X) + 0.5f,
                                    spawnY,
                                    static_cast<float>(SPAWN_Z) + 0.5f));
    m_Player->syncCamera(*m_Camera);

    m_World->Update(m_Camera->Position);
}

void Game::Render(int windowWidth, int windowHeight)
{
    m_World->Render(m_RenderMaster, *m_Camera);

    if (m_target.hit && m_outlineRenderer)
        m_outlineRenderer->Render(*m_Camera, m_target.blockPos);

    if (m_crosshairRenderer)
        m_crosshairRenderer->Render(windowWidth, windowHeight);
}

void Game::Update(float deltaTime)
{
    m_Player->update(*m_World, deltaTime);
    m_Player->syncCamera(*m_Camera);

    m_World->Update(m_Camera->Position);
    updateTargetBlock();
}

glm::vec3 Game::horizontalLookDir() const
{
    const float yaw = glm::radians(m_Camera->Yaw);
    return glm::normalize(glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw)));
}

void Game::updateTargetBlock()
{
    m_target = raycastWorld(*m_World, m_Camera->Position, m_Camera->Front, RAYCAST_REACH);
}

void Game::OnLeftClick()
{
    if (!m_target.hit)
        return;
    m_World->setBlock(m_target.blockPos.x, m_target.blockPos.y, m_target.blockPos.z,
                      ChunkBlock(BlockId::Air));
    updateTargetBlock();
}

void Game::OnRightClick()
{
    if (!m_target.hit)
        return;

    const glm::ivec3& p = m_target.previousPos;
    if (p.y < 0 || p.y >= WORLD_HEIGHT)
        return;

    if (m_World->getBlock(p.x, p.y, p.z) != BlockId::Air)
        return;

    // Do not place inside the player body
    if (m_Player->intersectsBlock(p.x, p.y, p.z))
        return;

    m_World->setBlock(p.x, p.y, p.z, ChunkBlock(m_placeBlock));
    updateTargetBlock();
}

void Game::ProcessInput(float deltaTime)
{
    (void)deltaTime;

    // Toggle fly mode (debug / creative)
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

    // Look with arrow keys
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

    if (Keys[GLFW_KEY_1] && !KeyProcessed[GLFW_KEY_1]) {
        m_placeBlock = BlockId::Stone;
        KeyProcessed[GLFW_KEY_1] = true;
    }
    if (Keys[GLFW_KEY_2] && !KeyProcessed[GLFW_KEY_2]) {
        m_placeBlock = BlockId::Dirt;
        KeyProcessed[GLFW_KEY_2] = true;
    }
    if (Keys[GLFW_KEY_3] && !KeyProcessed[GLFW_KEY_3]) {
        m_placeBlock = BlockId::Grass;
        KeyProcessed[GLFW_KEY_3] = true;
    }
    if (Keys[GLFW_KEY_4] && !KeyProcessed[GLFW_KEY_4]) {
        m_placeBlock = BlockId::Sand;
        KeyProcessed[GLFW_KEY_4] = true;
    }
    if (!Keys[GLFW_KEY_1]) KeyProcessed[GLFW_KEY_1] = false;
    if (!Keys[GLFW_KEY_2]) KeyProcessed[GLFW_KEY_2] = false;
    if (!Keys[GLFW_KEY_3]) KeyProcessed[GLFW_KEY_3] = false;
    if (!Keys[GLFW_KEY_4]) KeyProcessed[GLFW_KEY_4] = false;
}
