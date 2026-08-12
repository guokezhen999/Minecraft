//
// Created by 郭珂桢 on 2024/4/25.
//

#ifndef MINECRAFT_CONTEXT_H
#define MINECRAFT_CONTEXT_H

#include <SFML/Window/Window.hpp>

#include "Config.h"

struct Context
{
    Context(const Config &config);

    sf::Window *window;
};


#endif //MINECRAFT_CONTEXT_H
