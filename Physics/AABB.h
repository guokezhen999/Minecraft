//
// Axis-aligned bounding box helpers
//

#ifndef MINECRAFT_AABB_H
#define MINECRAFT_AABB_H

#include <glm/glm.hpp>
#include <algorithm>

struct AABB {
    glm::vec3 position{0.0f};   // Min corner
    glm::vec3 dimensions{1.0f};

    AABB() = default;
    AABB(const glm::vec3& pos, const glm::vec3& dim)
        : position(pos), dimensions(dim) {}

    explicit AABB(const glm::vec3& dim)
        : position(0.0f), dimensions(dim) {}

    glm::vec3 min() const { return position; }
    glm::vec3 max() const { return position + dimensions; }
    glm::vec3 center() const { return position + dimensions * 0.5f; }

    bool intersects(const AABB& other) const {
        const glm::vec3 aMax = max();
        const glm::vec3 bMax = other.max();
        return position.x < bMax.x && aMax.x > other.position.x &&
               position.y < bMax.y && aMax.y > other.position.y &&
               position.z < bMax.z && aMax.z > other.position.z;
    }

    bool intersectsBlock(int bx, int by, int bz) const {
        return intersects(AABB(glm::vec3(bx, by, bz), glm::vec3(1.0f)));
    }

    // Grow/shrink symmetrically on each axis by `pad`
    AABB expanded(float pad) const {
        return AABB(position - glm::vec3(pad), dimensions + glm::vec3(pad * 2.0f));
    }
};

#endif //MINECRAFT_AABB_H
