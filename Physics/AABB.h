//
// Created by 郭珂桢 on 25-8-1.
//

#ifndef MINECRAFT_AABB_H
#define MINECRAFT_AABB_H

#include <glm/glm.hpp>

struct AABB {
    AABB(const glm::vec3 &dim) : dimensions(dim) {}

    glm::vec3 position;
    const glm::vec3 dimensions;

};

#endif //MINECRAFT_AABB_H
