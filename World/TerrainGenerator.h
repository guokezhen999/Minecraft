//
// TerrainGenerator.h
// Continentality / temperature / moisture pick biomes; relief is shared then scaled.
//

#ifndef MINECRAFT_TERRAINGENERATOR_H
#define MINECRAFT_TERRAINGENERATOR_H

#include <cstdint>

// Surface biome from (C, T, M). Relief amplitude is blended separately.
enum class BiomeType {
    Ocean,
    Forest,
    Savanna,
    Desert,
    Tundra,
    Taiga,
    Jungle,
    TemperateDesert,
    Grassland,
    SnowyPlains
};

struct ClimateSample {
    float continent = 0.0f;    // C
    float temperature = 0.0f;  // T
    float moisture = 0.0f;     // M
};

struct TerrainColumn {
    int height = 0;
    BiomeType biome = BiomeType::Ocean;
    float moisture = 0.0f;
};

class TerrainGenerator {
public:
    explicit TerrainGenerator(int seed = 0);

    TerrainColumn sampleColumn(int worldX, int worldZ) const;

    // Terrain surface height at world column (worldX, worldZ)
    int getHeight(int worldX, int worldZ) const;

    // Block id (as int) at (worldX, y, worldZ)
    int getBlock(int worldX, int y, int worldZ) const;

    // Same as getBlock, but reuses a precomputed column height + biome (hot path)
    int getBlock(int worldX, int y, int worldZ, int height, BiomeType biome) const;

    BiomeType getBiome(int worldX, int worldZ) const;
    float getMoisture(int worldX, int worldZ) const;

    ClimateSample sampleClimate(int worldX, int worldZ) const;

    // True in desert interiors (barren — no cactus / dead shrub)
    bool isDeepDesert(int worldX, int worldZ) const;

    // Hash in [0, 1) for decoration rolls (stable per seed)
    float columnRoll(int worldX, int worldZ, uint32_t salt) const;

    // Moisture factor for plant density: clamp(0.35 + 0.65 * (M+1)/2, 0.2, 1)
    static float wetFactor(float moisture);

    // Returns true if a tree trunk base should be placed at (worldX, worldZ)
    bool shouldPlaceTree(int worldX, int worldZ) const;
    bool shouldPlaceTree(int worldX, int worldZ, const TerrainColumn& col) const;

    // Trunk height for the tree at (worldX, worldZ) — only call when shouldPlaceTree
    int getTreeHeight(int worldX, int worldZ) const;
    int getTreeHeight(int worldX, int worldZ, BiomeType biome) const;

    int getSeed() const { return m_seed; }

private:
    struct LandRelief {
        float plains = 0.0f;
        float hills = 0.0f;
        float mountains = 0.0f;
        float mountMask = 0.0f;
    };

    struct BiomeAmps {
        float hill = 1.0f;
        float mount = 1.0f;
        float desert = 0.0f;
    };

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

    static BiomeType biomeFromClimate(const ClimateSample& cl);
    static BiomeAmps blendedAmps(const ClimateSample& cl);

    LandRelief sampleLandRelief(float x, float z) const;
    float  oceanHeight(float x, float z) const;
    float  riverMask(float x, float z, float landW, float mountMask,
                     float mountAmp, float moisture) const;
    int    computeHeight(int worldX, int worldZ, const ClimateSample& cl) const;

    int m_seed;
};

#endif //MINECRAFT_TERRAINGENERATOR_H
