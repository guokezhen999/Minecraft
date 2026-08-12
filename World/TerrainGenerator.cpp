//
// TerrainGenerator.cpp
// Climate (Ocean/Grass/Desert) picks surface material.
// Independent relief noise adds plains ripple, rolling hills, and varied mountains.
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

static float smoothstepRange(float edge0, float edge1, float x) {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Climate thresholds (no "mountain biome" — relief is separate)
// Approximate shares on [-1,1]: ocean ~36%, grassland ~38%, desert ~26%
static constexpr float kThOceanLand   = -0.28f;
static constexpr float kThGrassDesert =  0.48f;
// Deep barren desert (no cactus / dead shrub) — core of large sand seas
static constexpr float kThDeepDesert  =  0.72f;

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

float TerrainGenerator::saltedValueNoise(float x, float z, uint32_t salt) const {
    const uint32_t s = static_cast<uint32_t>(m_seed) + salt;
    const float ox = static_cast<float>(wangHash(s)     & 0xFFFFu) * (1.0f / 64.0f);
    const float oz = static_cast<float>(wangHash(s + 1u) & 0xFFFFu) * (1.0f / 64.0f);
    return valueNoise(x + ox, z + oz);
}

float TerrainGenerator::saltedOctaveNoise(float x, float z, int octaves,
                                          float lacunarity, float persistence,
                                          uint32_t salt) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value    += saltedValueNoise(x * frequency, z * frequency, salt) * amplitude;
        maxVal   += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return value / maxVal;
}

float TerrainGenerator::treeNoise(int ix, int iz) const {
    uint32_t h = wangHash(static_cast<uint32_t>(ix * 198491317)
                        ^ wangHash(static_cast<uint32_t>(iz * 6542989))
                        ^ wangHash(static_cast<uint32_t>(m_seed + 999)));
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu) * 2.0f - 1.0f;
}

// ─── Constructor ─────────────────────────────────────────────────────────────

TerrainGenerator::TerrainGenerator(int seed) : m_seed(seed) {}

// ─── Climate (surface material) ──────────────────────────────────────────────

float TerrainGenerator::sampleClimateNoise(int worldX, int worldZ) const {
    const float bx = static_cast<float>(worldX) * BIOME_SCALE;
    const float bz = static_cast<float>(worldZ) * BIOME_SCALE;
    return saltedValueNoise(bx, bz, 1u);
}

BiomeType TerrainGenerator::getBiome(int worldX, int worldZ) const {
    const float n = sampleClimateNoise(worldX, worldZ);
    if (n < kThOceanLand)   return BiomeType::Ocean;
    if (n < kThGrassDesert) return BiomeType::Grassland;
    return BiomeType::Desert;
}

bool TerrainGenerator::isDeepDesert(int worldX, int worldZ) const {
    return sampleClimateNoise(worldX, worldZ) >= kThDeepDesert;
}

// ─── Height ──────────────────────────────────────────────────────────────────

float TerrainGenerator::oceanHeight(float x, float z) const {
    return saltedOctaveNoise(x * 0.020f, z * 0.020f, 3, 2.0f, 0.5f, 10u) * 6.0f
         + static_cast<float>(WATER_LEVEL) - 8.0f;
}

float TerrainGenerator::landHeight(float x, float z) const {
    // Gentle plains ripple
    const float plains =
        saltedOctaveNoise(x * 0.028f, z * 0.028f, 3, 2.0f, 0.50f, 20u) * 5.0f;

    // Rolling hills — common, modest height (≈0–16)
    const float hillN =
        saltedOctaveNoise(x * 0.032f, z * 0.032f, 4, 2.0f, 0.55f, 30u);
    const float hillPos = std::max(0.0f, hillN);
    const float hills = hillPos * hillPos * 16.0f;

    // Mountain belt mask — large-scale, sparse (only strong peaks become ranges)
    const float mountMaskN =
        saltedOctaveNoise(x * 0.0065f, z * 0.0065f, 4, 2.0f, 0.58f, 40u);
    const float mountMask = smoothstepRange(0.22f, 0.58f, mountMaskN);

    // Ridge shape inside mountain belts
    const float ridgeBase =
        saltedOctaveNoise(x * 0.011f, z * 0.011f, 5, 2.0f, 0.58f, 50u);
    const float ridge = 1.0f - std::abs(ridgeBase);

    // Per-range peak amplitude → short vs tall mountains (~18–48)
    const float peakVar =
        saltedOctaveNoise(x * 0.0035f, z * 0.0035f, 3, 2.0f, 0.50f, 60u);
    const float peakAmp = 18.0f + (peakVar * 0.5f + 0.5f) * 30.0f;

    const float detail =
        saltedOctaveNoise(x * 0.042f, z * 0.042f, 3, 2.0f, 0.45f, 70u) * 7.0f;

    const float mountains = mountMask * (ridge * peakAmp + detail);

    return static_cast<float>(WATER_LEVEL) + 2.0f + plains + hills + mountains;
}

int TerrainGenerator::getHeight(int worldX, int worldZ) const {
    const float x = static_cast<float>(worldX);
    const float z = static_cast<float>(worldZ);
    const float n = sampleClimateNoise(worldX, worldZ);
    const float T = BIOME_BLEND;

    // Ocean ↔ land blend only; grass/desert share the same relief stack so
    // sand mountains and grass mountains meet without cliffs.
    float h;
    if (n < kThOceanLand - T) {
        h = oceanHeight(x, z);
    } else if (n < kThOceanLand + T) {
        const float t = smoothstepRange(kThOceanLand - T, kThOceanLand + T, n);
        h = lerp(oceanHeight(x, z), landHeight(x, z), t);
    } else {
        h = landHeight(x, z);
        // Tiny desert dune bias (does not create cliffs at grass↔desert)
        if (n > kThGrassDesert - T) {
            const float desertW = (n < kThGrassDesert + T)
                ? smoothstepRange(kThGrassDesert - T, kThGrassDesert + T, n)
                : 1.0f;
            const float dunes =
                saltedOctaveNoise(x * 0.040f, z * 0.040f, 2, 2.0f, 0.4f, 80u) * 2.5f;
            h += desertW * dunes;
        }
    }

    return std::clamp(static_cast<int>(h), 2, WORLD_HEIGHT - 16);
}

// ─── Block (surface follows climate only) ────────────────────────────────────

int TerrainGenerator::getBlock(int worldX, int y, int worldZ) const {
    return getBlock(worldX, y, worldZ, getHeight(worldX, worldZ), getBiome(worldX, worldZ));
}

int TerrainGenerator::getBlock(int worldX, int y, int worldZ,
                               int height, BiomeType biome) const {
    (void)worldX;
    (void)worldZ;

    if (y > height) {
        if (y <= WATER_LEVEL) return static_cast<int>(BlockId::Water);
        return static_cast<int>(BlockId::Air);
    }

    // Surface follows climate only (sand / grass) — never force stone peaks
    if (y == height) {
        switch (biome) {
            case BiomeType::Ocean:
            case BiomeType::Desert:
                return static_cast<int>(BlockId::Sand);
            case BiomeType::Grassland:
                return (height <= WATER_LEVEL + 1)
                    ? static_cast<int>(BlockId::Sand)
                    : static_cast<int>(BlockId::Grass);
        }
    }

    if (y >= height - 4) {
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
    const BiomeType biome = getBiome(worldX, worldZ);
    if (biome != BiomeType::Grassland)
        return false;

    const int height = getHeight(worldX, worldZ);
    if (height <= WATER_LEVEL + 1) return false;

    if (getBlock(worldX, height, worldZ, height, biome)
        != static_cast<int>(BlockId::Grass))
        return false;

    // Sparser on taller grass hills/mountains
    const float tn = treeNoise(worldX, worldZ);
    if (height > WATER_LEVEL + 18)
        return tn > 0.97f;
    return tn > 0.94f;
}

int TerrainGenerator::getTreeHeight(int worldX, int worldZ) const {
    float tn = treeNoise(worldX + 3000, worldZ + 7000);
    int h = static_cast<int>((tn + 1.0f) * 0.5f * 3.0f) + 4; // [4, 6]
    return h;
}
