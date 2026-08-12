//
// TerrainGenerator.h
// Climate (surface) and relief (hills/mountains) are independent noise axes.
//

#ifndef MINECRAFT_TERRAINGENERATOR_H
#define MINECRAFT_TERRAINGENERATOR_H

#include <cstdint>

// Surface / climate biome — NOT tied to elevation shape
enum class BiomeType {
    Ocean,
    Grassland,
    Desert
};

class TerrainGenerator {
public:
    explicit TerrainGenerator(int seed = 0);

    // Terrain surface height at world column (worldX, worldZ)
    int getHeight(int worldX, int worldZ) const;

    // Block id (as int) at (worldX, y, worldZ)
    int getBlock(int worldX, int y, int worldZ) const;

    // Same as getBlock, but reuses a precomputed column height + biome (hot path)
    int getBlock(int worldX, int y, int worldZ, int height, BiomeType biome) const;

    // Climate biome at world column (sand / grass / ocean)
    BiomeType getBiome(int worldX, int worldZ) const;

    // True in desert interiors (barren — no cactus / dead shrub)
    bool isDeepDesert(int worldX, int worldZ) const;

    // Returns true if a tree trunk base should be placed at (worldX, worldZ)
    bool shouldPlaceTree(int worldX, int worldZ) const;

    // Trunk height for the tree at (worldX, worldZ) — only call when shouldPlaceTree
    int getTreeHeight(int worldX, int worldZ) const;

    int getSeed() const { return m_seed; }

private:
    static uint32_t wangHash(uint32_t v);
    float  whiteNoise(int ix, int iz) const;
    float  valueNoise(float x, float z) const;
    float  octaveNoise(float x, float z, int octaves,
                       float lacunarity, float persistence) const;

    // Salted 2D noise (seed + salt → lattice offset) so layers stay uncorrelated
    float  saltedValueNoise(float x, float z, uint32_t salt) const;
    float  saltedOctaveNoise(float x, float z, int octaves,
                             float lacunarity, float persistence,
                             uint32_t salt) const;

    float  treeNoise(int ix, int iz) const;

    float  sampleClimateNoise(int worldX, int worldZ) const;
    float  oceanHeight(float x, float z) const;
    float  landHeight(float x, float z) const;   // plains + hills + mountains

    int m_seed;
};

#endif //MINECRAFT_TERRAINGENERATOR_H
