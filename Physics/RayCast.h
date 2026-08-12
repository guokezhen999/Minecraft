//
// Voxel raycast (Amanatides & Woo grid traversal)
//

#ifndef MINECRAFT_RAYCAST_H
#define MINECRAFT_RAYCAST_H

#include <glm/glm.hpp>

class World;

struct RaycastHit {
    bool hit = false;
    glm::ivec3 blockPos{0};     // Block that was hit (break target)
    glm::ivec3 previousPos{0};  // Last empty cell before hit (place target)
    glm::ivec3 faceNormal{0};   // Outward normal of the hit face
};

// Cast from origin along direction (need not be normalized).
// Always skips air. When hitFluids is false, also skips water so aim
// can reach solids underwater; when true, water is a valid target.
RaycastHit raycastWorld(const World& world,
                        const glm::vec3& origin,
                        const glm::vec3& direction,
                        float maxDistance,
                        bool hitFluids = false);

#endif //MINECRAFT_RAYCAST_H
