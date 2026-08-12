//
// Frustum.h – extract view frustum from proj*view and test AABBs
//

#ifndef MINECRAFT_FRUSTUM_H
#define MINECRAFT_FRUSTUM_H

#include <cmath>
#include <glm/glm.hpp>

struct Frustum {
    // plane = (nx, ny, nz, d) with ax+by+cz+d >= 0 inside
    glm::vec4 planes[6];

    void update(const glm::mat4& m) {
        // Gribb/Hartmann: rows of clip matrix
        const glm::vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
        const glm::vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
        const glm::vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
        const glm::vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

        planes[0] = row3 + row0; // left
        planes[1] = row3 - row0; // right
        planes[2] = row3 + row1; // bottom
        planes[3] = row3 - row1; // top
        planes[4] = row3 + row2; // near
        planes[5] = row3 - row2; // far

        for (auto& p : planes) {
            const float len = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
            if (len > 1e-6f)
                p /= len;
        }
    }

    bool intersectsAABB(const glm::vec3& minB, const glm::vec3& maxB) const {
        for (const auto& p : planes) {
            // positive vertex along plane normal
            const glm::vec3 positive(
                p.x >= 0.0f ? maxB.x : minB.x,
                p.y >= 0.0f ? maxB.y : minB.y,
                p.z >= 0.0f ? maxB.z : minB.z);
            if (p.x * positive.x + p.y * positive.y + p.z * positive.z + p.w < 0.0f)
                return false;
        }
        return true;
    }

    bool intersectsChunkColumn(int cx, int cz, int chunkSize) const {
        const float s = static_cast<float>(chunkSize);
        const glm::vec3 minB(cx * s, 0.0f, cz * s);
        const glm::vec3 maxB((cx + 1) * s, s, (cz + 1) * s);
        return intersectsAABB(minB, maxB);
    }
};

#endif //MINECRAFT_FRUSTUM_H
