//
// Created by 郭珂桢 on 2024/4/25.
//

#ifndef MINECRAFT_CONFIG_H
#define MINECRAFT_CONFIG_H

#include <string>


struct Config
{
    int windowX = 1280;
    int windowY = 720;
    int windowPosX = 100;
    int windowPosY = 100;
    bool isFullscreen = false;
    int renderDistance = 10;
    int fov = 90;
    float mouseSensitivity = 0.10f;
    bool vsync = true;
    bool showSunMoon = true;
    std::string lastWorld;
};


#endif //MINECRAFT_CONFIG_H
