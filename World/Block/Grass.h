//
// Grass is one placeable block. Biome only changes how it looks.
//

#ifndef MINECRAFT_GRASS_H
#define MINECRAFT_GRASS_H

#include "BlockId.h"
#include "../TerrainGenerator.h"

inline bool isGrassBlock(BlockId id)
{
    return id == BlockId::Grass
        || id == BlockId::SavannaGrass
        || id == BlockId::TaigaGrass
        || id == BlockId::TundraGrass;
}

inline BlockId grassAppearance(BiomeType biome)
{
    switch (biome) {
    case BiomeType::Savanna: return BlockId::SavannaGrass;
    case BiomeType::Taiga:   return BlockId::TaigaGrass;
    case BiomeType::Tundra:  return BlockId::TundraGrass;
    default:                 return BlockId::Grass;
    }
}

inline BlockId canonicalizePlaceable(BlockId id)
{
    const int n = static_cast<int>(id);
    if (n < 0 || n >= static_cast<int>(BlockId::NUM_TYPES))
        return BlockId::Air;
    if (isGrassBlock(id))
        return BlockId::Grass;
    return id;
}

#endif
