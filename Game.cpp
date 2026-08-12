#include "Game.h"

#include "Renderer/CubeRenderer.h"
#include "ResourceManager.h"
#include "World/World.h"
#include "World/WorldConstants.h"
#include "World/Block/BlockDataBase.h"

Game::Game(Config &config, Camera &m_Camera)
: m_Config(config), m_Camera(&m_Camera), Keys(), KeyProcessed()
{
}

void Game::Init()
{
    // Ensure block DB / atlas exists before the world worker touches meshes
    BlockDatabase::Get();

    // Load shaders
    ResourceManager::BlockShader = new BasicShader("resources/shaders/basic.vert",
                                               "resources/shaders/basic.frag");
    ResourceManager::BlockShader->Use();
    ResourceManager::BlockShader->SetInteger("texture1", 0);
    // Load textures
    ResourceManager::BlockTexture = new BasicTexture("resources/textures/defaultPack.png");

    //  renderer
    m_RenderMaster.InitCubeRenderer();

    // Spawn above water so the single-layer world is visible immediately
    m_Camera->SetPosition(glm::vec3(0.5f, static_cast<float>(WATER_LEVEL) + 10.0f, 0.5f));

    // World seed — change this number to get a completely different terrain
    constexpr int WORLD_SEED = 114514;
    m_World = std::make_unique<World>(WORLD_SEED);
    m_World->Update(m_Camera->Position);

}

void Game::Render()
{
    m_World->Render(m_RenderMaster, *m_Camera);
}

void Game::Update()
{
    m_World->Update(m_Camera->Position);
}

void Game::ProcessInput(float deltaTime)
{
    if (this->Keys[sf::Keyboard::W])
        m_Camera->ProcessKeyboard(FORWARD, deltaTime);
    if (this->Keys[sf::Keyboard::S])
        m_Camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (this->Keys[sf::Keyboard::A])
        m_Camera->ProcessKeyboard(LEFT, deltaTime);
    if (this->Keys[sf::Keyboard::D])
        m_Camera->ProcessKeyboard(RIGHT, deltaTime);
    if (this->Keys[sf::Keyboard::R])
        m_Camera->ProcessKeyboard(UP, deltaTime);
    if (this->Keys[sf::Keyboard::F])
        m_Camera->ProcessKeyboard(DOWN, deltaTime);

    // 方向键控制视角旋转（与鼠标共用同一套接口）
    const float ARROW_LOOK_SPEED = 120.0f; // 每秒转动角度，可调节
    float lookDelta = ARROW_LOOK_SPEED * deltaTime;
    if (this->Keys[sf::Keyboard::Up])
        m_Camera->ProcessMouseMovement(0.0f,  lookDelta);   // 向上看
    if (this->Keys[sf::Keyboard::Down])
        m_Camera->ProcessMouseMovement(0.0f, -lookDelta);   // 向下看
    if (this->Keys[sf::Keyboard::Left])
        m_Camera->ProcessMouseMovement(-lookDelta, 0.0f);   // 向左看
    if (this->Keys[sf::Keyboard::Right])
        m_Camera->ProcessMouseMovement( lookDelta, 0.0f);   // 向右看
}
