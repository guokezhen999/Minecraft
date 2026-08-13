//
// LightEngine – sky columns, 6-way propagate, local column relight
// Light maps live on ChunkSection; never written during buildMesh.
//

#ifndef MINECRAFT_LIGHTENGINE_H
#define MINECRAFT_LIGHTENGINE_H

#include <utility>
#include <vector>
#include <cstdint>

#include "Block/ChunkBlock.h"

class World;
class Chunk;

class LightEngine {
public:
    // Isolated column (chunk not yet in the world). Worker-safe.
    static void computeColumn(Chunk& chunk);

    // After a column is inserted. Unique lock. Increase-only bleed across borders.
    static void propagateFromNeighbors(World& world, int cx, int cz,
                                       std::vector<std::pair<int, int>>& remeshCols);

    // Player edit: local radius-16 box, not whole columns.
    static void updateAfterEdit(World& world, int x, int y, int z,
                                std::vector<std::pair<int, int>>& remeshCols);

    // Fluid batch: recompute the given columns then bleed to neighbors.
    static void relightColumns(World& world,
                               const std::vector<std::pair<int, int>>& cols,
                               std::vector<std::pair<int, int>>& remeshCols);

    static uint8_t opacityOf(ChunkBlock block);
    static uint8_t emissionOf(ChunkBlock block);
};

#endif // MINECRAFT_LIGHTENGINE_H
