//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_ENTITY_H
#define MINECRAFT_ENTITY_H

#include "glm/glm.hpp"

struct Entity
{
    Entity()
    : Position(glm::vec3(0.0f)),
    Rotation(glm::vec3(0.0f)),
    Velocity(glm::vec3(0.0f))
    {}

    Entity(const glm::vec3 &pos, const glm::vec3 &rot)
    : Position(pos), Rotation(rot), Velocity(glm::vec3(0.0f))
    {}

    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Velocity;


};

#endif //MINECRAFT_ENTITY_H
