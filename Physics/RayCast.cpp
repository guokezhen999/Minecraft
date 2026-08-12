//
// Voxel raycast (Amanatides & Woo grid traversal)
//

#include "RayCast.h"
#include "../World/World.h"
#include "../World/Block/BlockData.h"

#include <cmath>

namespace {

bool isRayTarget(ChunkBlock block) {
    if (block == BlockId::Air)
        return false;
    // Pass through water / other liquids
    if (block.GetData().shaderType == BlockShaderType::Liquid)
        return false;
    return true;
}

int floorToInt(float v) {
    return static_cast<int>(std::floor(v));
}

} // namespace

RaycastHit raycastWorld(const World& world,
                        const glm::vec3& origin,
                        const glm::vec3& direction,
                        float maxDistance) {
    RaycastHit result;

    const float len = glm::length(direction);
    if (len < 1e-6f || maxDistance <= 0.0f)
        return result;

    const glm::vec3 dir = direction / len;

    int x = floorToInt(origin.x);
    int y = floorToInt(origin.y);
    int z = floorToInt(origin.z);

    const int stepX = dir.x > 0.0f ? 1 : (dir.x < 0.0f ? -1 : 0);
    const int stepY = dir.y > 0.0f ? 1 : (dir.y < 0.0f ? -1 : 0);
    const int stepZ = dir.z > 0.0f ? 1 : (dir.z < 0.0f ? -1 : 0);

    // Distance along ray to cross next voxel boundary on each axis
    const float tDeltaX = stepX != 0 ? std::abs(1.0f / dir.x) : 1e30f;
    const float tDeltaY = stepY != 0 ? std::abs(1.0f / dir.y) : 1e30f;
    const float tDeltaZ = stepZ != 0 ? std::abs(1.0f / dir.z) : 1e30f;

    auto nextBoundaryT = [](float pos, float d, int voxel, int step) -> float {
        if (step == 0)
            return 1e30f;
        if (step > 0)
            return (static_cast<float>(voxel + 1) - pos) / d;
        return (pos - static_cast<float>(voxel)) / -d;
    };

    float tMaxX = nextBoundaryT(origin.x, dir.x, x, stepX);
    float tMaxY = nextBoundaryT(origin.y, dir.y, y, stepY);
    float tMaxZ = nextBoundaryT(origin.z, dir.z, z, stepZ);

    glm::ivec3 prev{x, y, z};
    float t = 0.0f;

    // If already inside a solid block, step out first without counting as hit
    // (so flying camera inside terrain can still aim outward).
    const int maxSteps = static_cast<int>(maxDistance * 3.0f) + 3;
    for (int i = 0; i < maxSteps && t <= maxDistance; ++i) {
        ChunkBlock block = world.getBlock(x, y, z);
        if (isRayTarget(block)) {
            // Ignore a hit at t≈0 (camera inside the block)
            if (t > 1e-4f) {
                result.hit = true;
                result.blockPos = {x, y, z};
                result.previousPos = prev;
                return result;
            }
        }

        prev = {x, y, z};

        // Advance to next voxel; record which face we crossed
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                t = tMaxX;
                x += stepX;
                tMaxX += tDeltaX;
                result.faceNormal = {-stepX, 0, 0};
            } else {
                t = tMaxZ;
                z += stepZ;
                tMaxZ += tDeltaZ;
                result.faceNormal = {0, 0, -stepZ};
            }
        } else {
            if (tMaxY < tMaxZ) {
                t = tMaxY;
                y += stepY;
                tMaxY += tDeltaY;
                result.faceNormal = {0, -stepY, 0};
            } else {
                t = tMaxZ;
                z += stepZ;
                tMaxZ += tDeltaZ;
                result.faceNormal = {0, 0, -stepZ};
            }
        }
    }

    return result;
}
