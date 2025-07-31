//
// Created by 郭珂桢 on 2024/4/21.
//

#ifndef MINECRAFT_GAME_H
#define MINECRAFT_GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Config.h"
#include "Context.h"
#include "Camera.h"
#include "Renderer/RenderMaster.h"

#include <iostream>

class Game
{
public:
    Game(Config &config, Camera &camera);

    void Init();
    void Render();
    void Update();
    void ProcessInput(float deltaTime);

    Camera *m_Camera;
    RenderMaster m_RenderMaster;

    const Config &m_Config;

    GLboolean Keys[1024];
    bool KeyProcessed[1024];
    bool IsPopState = false;
};

#endif //MINECRAFT_GAME_H
