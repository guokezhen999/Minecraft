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
#include "Renderer/SkyRenderer.h"
#include "Physics/RayCast.h"
#include "Physics/Player.h"
#include "World/World.h"
#include "World/Block/BlockId.h"
#include "UI/Hotbar.h"

#include <iostream>
#include <memory>

class Game
{
public:
    Game(Config &config, Camera &camera);

    void Init();
    void Render(int windowWidth, int windowHeight);
    void Update(float deltaTime);
    void ProcessInput(float deltaTime);

    void OnLeftClick();   // break
    void OnRightClick();  // place
    void OnScroll(float yoffset); // hotbar cycle

    bool isCameraUnderwater() const { return m_cameraUnderwater; }
    glm::vec3 clearColor() const;

    Camera *m_Camera;
    RenderMaster m_RenderMaster;

    const Config &m_Config;

    GLboolean Keys[1024];
    bool KeyProcessed[1024];
    bool IsPopState = false;

    std::unique_ptr<World> m_World;
    std::unique_ptr<Player> m_Player;

private:
    void updateTargetBlock();
    glm::vec3 horizontalLookDir() const;

    std::unique_ptr<OutlineRenderer> m_outlineRenderer;
    std::unique_ptr<CrosshairRenderer> m_crosshairRenderer;
    std::unique_ptr<HotbarRenderer> m_hotbarRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    Hotbar m_hotbar;
    RaycastHit m_target;
    bool m_cameraUnderwater = false;
    float m_tickAcc = 0.0f;
};

#endif //MINECRAFT_GAME_H
