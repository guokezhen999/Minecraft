#include "Game.h"

#include "Renderer/CubeRenderer.h"
#include "ResourceManager.h"

Game::Game(Config &config, Camera &m_Camera)
: m_Config(config), m_Camera(&m_Camera), Keys(), KeyProcessed()
{
}

void Game::Init()
{
    // Load shaders
    ResourceManager::BlockShader = BasicShader("resources/shaders/basic.vert",
                                               "resources/shaders/basic.frag");
    ResourceManager::BlockShader.Use();
    ResourceManager::BlockShader.SetInteger("texture1", 0);
    // Load textures
    ResourceManager::BlockTexture = ResourceManager::LoadTextureFromFile("resources/textures/defaultPack.png");

    //  renderer
    m_RenderMaster.InitCubeRenderer();

}

void Game::Render()
{
    m_RenderMaster.RenderBlocks(*m_Camera);
}

void Game::Update()
{

}

void Game::ProcessInput(float deltaTime)
{
    if (this->Keys[GLFW_KEY_W])
        m_Camera->ProcessKeyboard(FORWARD, deltaTime);
    if (this->Keys[GLFW_KEY_S])
        m_Camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (this->Keys[GLFW_KEY_A])
        m_Camera->ProcessKeyboard(LEFT, deltaTime);
    if (this->Keys[GLFW_KEY_D])
        m_Camera->ProcessKeyboard(RIGHT, deltaTime);
    if (this->Keys[GLFW_KEY_R])
        m_Camera->ProcessKeyboard(UP, deltaTime);
    if (this->Keys[GLFW_KEY_F])
        m_Camera->ProcessKeyboard(DOWN, deltaTime);
}





