//
// TerrainGenerator.h
// Seed-based, multi-biome terrain height + block generator using Wang-hash value noise
//

#ifndef MINECRAFT_TERRAINGENERATOR_H
#define MINECRAFT_TERRAINGENERATOR_H

#include <cstdint>

enum class BiomeType {
    Ocean,
    Grassland,
    Desert,
    Mountains
};

class TerrainGenerator {
public:
    explicit TerrainGenerator(int seed = 0);

    // Terrain surface height at world column (worldX, worldZ)
    int getHeight(int worldX, int worldZ) const;

    // Block id (as int) at (worldX, y, worldZ)
    int getBlock(int worldX, int y, int worldZ) const;

    // Biome at world column
    BiomeType getBiome(int worldX, int worldZ) const;

    // Returns true if a tree trunk base should be placed at (worldX, worldZ)
    bool shouldPlaceTree(int worldX, int worldZ) const;

    // Trunk height for the tree at (worldX, worldZ) — only call when shouldPlaceTree
    int getTreeHeight(int worldX, int worldZ) const;

    int getSeed() const { return m_seed; }

private:
    // --- Noise primitives ---
    static uint32_t wangHash(uint32_t v);
    float  whiteNoise(int ix, int iz) const;           // lattice pseudo-random [-1,1]
    float  valueNoise(float x, float z) const;         // smooth bilinear value noise [-1,1]
    float  octaveNoise(float x, float z, int octaves,
                       float lacunarity, float persistence) const;

    // Separate noise seeded with an extra salt for biome / tree decisions
    float  biomeNoise(float x, float z) const;
    float  treeNoise(int ix, int iz) const;            // [-1,1] per-column

    int m_seed;
};

#endif //MINECRAFT_TERRAINGENERATOR_H
