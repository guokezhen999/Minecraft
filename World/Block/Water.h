//
// Water fluid helpers: levels, height, replaceability
// Level 0 = source (full). Levels 1–7 = flowing (increasingly shallow).
//

#ifndef MINECRAFT_WATER_H
#define MINECRAFT_WATER_H

#include "ChunkBlock.h"
#include "BlockId.h"
#include "BlockData.h"

#include <cmath>

namespace Water {

constexpr int MAX_LEVEL = 7;

inline bool isWater(ChunkBlock b) {
    return b.id == static_cast<Block_t>(BlockId::Water);
}

inline int level(ChunkBlock b) {
    return isWater(b) ? static_cast<int>(b.meta & 0x7u) : MAX_LEVEL + 1;
}

inline bool isSource(ChunkBlock b) {
    return isWater(b) && level(b) == 0;
}

inline ChunkBlock make(int lvl) {
    ChunkBlock b(BlockId::Water);
    if (lvl < 0) lvl = 0;
    if (lvl > MAX_LEVEL) lvl = MAX_LEVEL;
    b.meta = static_cast<uint8_t>(lvl);
    return b;
}

inline ChunkBlock makeSource() {
    return make(0);
}

// Visual surface height in [0,1] within the block cell.
// Water with water above fills the cell so columns look continuous.
inline float surfaceHeight(ChunkBlock b, ChunkBlock above) {
    if (!isWater(b))
        return 0.0f;
    if (isWater(above))
        return 1.0f;
    const int lvl = level(b);
    if (lvl == 0)
        return 1.0f;
    return static_cast<float>(MAX_LEVEL + 1 - lvl) / static_cast<float>(MAX_LEVEL + 1);
}

inline bool isReplaceable(ChunkBlock b) {
    if (b == BlockId::Air)
        return true;
    if (isWater(b))
        return true;
    // Plants can be washed away / overwritten
    return b.GetData().meshType == BlockMeshType::X;
}

// Air, plants, or shallower water (higher level number) can receive flow.
inline bool canFlowInto(ChunkBlock b) {
    if (b == BlockId::Air)
        return true;
    if (b.GetData().meshType == BlockMeshType::X)
        return true;
    return isWater(b);
}

// Infinite-source support: solid (or source water) beneath the cell.
inline bool hasInfiniteSupportBelow(ChunkBlock below, int worldY) {
    if (worldY <= 0)
        return true;
    if (isSource(below))
        return true;
    if (isWater(below) || below == BlockId::Air)
        return false;
    return below.GetData().isCollidable;
}

// True when resting on solid ground (not air / water) — only then water spreads sideways.
inline bool isOnSolidGround(ChunkBlock below, int worldY) {
    if (worldY <= 0)
        return true;
    return !canFlowInto(below);
}

// Eye / camera inside a water cell below its surface height.
inline bool isSubmerged(ChunkBlock inCell, ChunkBlock above, float worldY) {
    if (!isWater(inCell))
        return false;
    const float h = surfaceHeight(inCell, above);
    const float localY = worldY - std::floor(worldY);
    return localY < h - 0.001f;
}

} // namespace Water

#endif // MINECRAFT_WATER_H
