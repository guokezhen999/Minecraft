//
// TerrainGenerator.cpp
// Seed-based terrain generation with biomes, Wang-hash value noise, and tree placement
// Heights are scaled for a single-layer CHUNK_SIZE world with WATER_LEVEL near mid-height.
//

#include "TerrainGenerator.h"
#include "WorldConstants.h"
#include "Block/BlockId.h"

#include <cmath>
#include <algorithm>

// ─── Wang hash ───────────────────────────────────────────────────────────────

uint32_t TerrainGenerator::wangHash(uint32_t v) {
    v = (v ^ 61u) ^ (v >> 16u);
    v *= 9u;
    v ^= v >> 4u;
    v *= 0x27d4eb2du;
    v ^= v >> 15u;
    return v;
}

// ─── Lattice noise ───────────────────────────────────────────────────────────

float TerrainGenerator::whiteNoise(int ix, int iz) const {
    uint32_t h = wangHash(static_cast<uint32_t>(ix * 73856093)
                        ^ wangHash(static_cast<uint32_t>(iz * 19349663))
                        ^ wangHash(static_cast<uint32_t>(m_seed)));
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu) * 2.0f - 1.0f;
}

static float smoothstep(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerp(float a, float b, float t) { return a + t * (b - a); }

float TerrainGenerator::valueNoise(float x, float z) const {
    int ix = static_cast<int>(std::floor(x));
    int iz = static_cast<int>(std::floor(z));
    float fx = smoothstep(x - static_cast<float>(ix));
    float fz = smoothstep(z - static_cast<float>(iz));

    float v00 = whiteNoise(ix,     iz    );
    float v10 = whiteNoise(ix + 1, iz    );
    float v01 = whiteNoise(ix,     iz + 1);
    float v11 = whiteNoise(ix + 1, iz + 1);

    return lerp(lerp(v00, v10, fx), lerp(v01, v11, fx), fz);
}

float TerrainGenerator::octaveNoise(float x, float z, int octaves,
                                    float lacunarity, float persistence) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value    += valueNoise(x * frequency, z * frequency) * amplitude;
        maxVal   += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return value / maxVal;
}

float TerrainGenerator::biomeNoise(float x, float z) const {
    uint32_t savedSeed = static_cast<uint32_t>(m_seed);
    return valueNoise(x + static_cast<float>(wangHash(savedSeed + 1u)),
                      z + static_cast<float>(wangHash(savedSeed + 2u)));
}

float TerrainGenerator::treeNoise(int ix, int iz) const {
    uint32_t h = wangHash(static_cast<uint32_t>(ix * 198491317)
                        ^ wangHash(static_cast<uint32_t>(iz * 6542989))
                        ^ wangHash(static_cast<uint32_t>(m_seed + 999)));
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu) * 2.0f - 1.0f;
}

// ─── Constructor ─────────────────────────────────────────────────────────────

TerrainGenerator::TerrainGenerator(int seed) : m_seed(seed) {}

// ─── Biome ───────────────────────────────────────────────────────────────────

BiomeType TerrainGenerator::getBiome(int worldX, int worldZ) const {
    float bx = worldX * 0.003f;
    float bz = worldZ * 0.003f;
    float n = biomeNoise(bx, bz);

    if (n < -0.30f) return BiomeType::Ocean;
    if (n < -0.05f) return BiomeType::Grassland;
    if (n <  0.35f) return BiomeType::Desert;
    return BiomeType::Mountains;
}

// ─── Height ──────────────────────────────────────────────────────────────────

int TerrainGenerator::getHeight(int worldX, int worldZ) const {
    BiomeType biome = getBiome(worldX, worldZ);

    float x = static_cast<float>(worldX);
    float z = static_cast<float>(worldZ);

    float h;
    switch (biome) {
        case BiomeType::Ocean:
            h = octaveNoise(x * 0.020f, z * 0.020f, 3, 2.0f, 0.5f) * 2.0f
                + static_cast<float>(WATER_LEVEL) - 4.0f;
            break;

        case BiomeType::Grassland:
            h = octaveNoise(x * 0.025f, z * 0.025f, 4, 2.0f, 0.55f) * 3.0f
                + static_cast<float>(WATER_LEVEL) + 1.0f;
            break;

        case BiomeType::Desert:
            h = octaveNoise(x * 0.022f, z * 0.022f, 3, 2.0f, 0.45f) * 2.0f
                + static_cast<float>(WATER_LEVEL);
            break;

        case BiomeType::Mountains:
        default:
            {
                float base = octaveNoise(x * 0.015f, z * 0.015f, 5, 2.0f, 0.60f);
                float ridge = 1.0f - std::abs(base);
                h = ridge * 6.0f + static_cast<float>(WATER_LEVEL) + 1.0f
                    + octaveNoise(x * 0.050f, z * 0.050f, 2, 2.0f, 0.4f) * 1.5f;
            }
            break;
    }

    int height = static_cast<int>(h);
    // Leave headroom for surface + short trees / flora within the single layer
    return std::clamp(height, 2, CHUNK_SIZE - 3);
}

// ─── Block ───────────────────────────────────────────────────────────────────

int TerrainGenerator::getBlock(int worldX, int y, int worldZ) const {
    int height = getHeight(worldX, worldZ);
    BiomeType biome = getBiome(worldX, worldZ);

    if (y > height) {
        if (y <= WATER_LEVEL) return static_cast<int>(BlockId::Water);
        return static_cast<int>(BlockId::Air);
    }

    if (y == height) {
        switch (biome) {
            case BiomeType::Ocean:
            case BiomeType::Desert:
                return static_cast<int>(BlockId::Sand);
            case BiomeType::Grassland:
                return (height <= WATER_LEVEL + 1)
                    ? static_cast<int>(BlockId::Sand)
                    : static_cast<int>(BlockId::Grass);
            case BiomeType::Mountains:
                return (height >= WATER_LEVEL + 5)
                    ? static_cast<int>(BlockId::Stone)
                    : static_cast<int>(BlockId::Grass);
        }
    }

    if (y >= height - 3) {
        switch (biome) {
            case BiomeType::Desert:
            case BiomeType::Ocean:
                return static_cast<int>(BlockId::Sand);
            default:
                return static_cast<int>(BlockId::Dirt);
        }
    }

    return static_cast<int>(BlockId::Stone);
}

// ─── Tree placement ──────────────────────────────────────────────────────────

bool TerrainGenerator::shouldPlaceTree(int worldX, int worldZ) const {
    BiomeType biome = getBiome(worldX, worldZ);

    int height = getHeight(worldX, worldZ);
    if (biome == BiomeType::Ocean) return false;
    if (height <= WATER_LEVEL + 1) return false;

    float tn = treeNoise(worldX, worldZ);

    switch (biome) {
        case BiomeType::Grassland:
            return tn > 0.82f;
        case BiomeType::Mountains:
            return tn > 0.88f && height < WATER_LEVEL + 5;
        case BiomeType::Desert:
            return false;
        default:
            return false;
    }
}

int TerrainGenerator::getTreeHeight(int worldX, int worldZ) const {
    float tn = treeNoise(worldX + 3000, worldZ + 7000);
    int h = static_cast<int>((tn + 1.0f) * 0.5f * 3.0f) + 4; // [4, 6]
    return h;
}
