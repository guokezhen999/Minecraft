//
// Created by 郭珂桢 on 2024/4/25.
//

#ifndef MINECRAFT_CONTEXT_H
#define MINECRAFT_CONTEXT_H

#include <GLFW/glfw3.h>

#include "Config.h"

struct Context
{
    Context(const Config &config);

    GLFWwindow *Window;
};


#endif //MINECRAFT_CONTEXT_H
