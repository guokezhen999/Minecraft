//
// TerrainGenerator.cpp
// Continentality / temperature / moisture pick biomes.
// Shared relief (plains + hills + mountains) is scaled per biome, then rivers carve down.
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

float TerrainGenerator::columnRoll(int worldX, int worldZ, uint32_t salt) const {
    uint32_t h = wangHash(static_cast<uint32_t>(worldX * 198491317)
                        ^ wangHash(static_cast<uint32_t>(worldZ * 6542989))
                        ^ wangHash(static_cast<uint32_t>(m_seed) + salt));
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

float TerrainGenerator::wetFactor(float moisture) {
    return std::clamp(0.35f + 0.65f * (moisture + 1.0f) * 0.5f, 0.2f, 1.0f);
}

// ─── Constructor ─────────────────────────────────────────────────────────────

TerrainGenerator::TerrainGenerator(int seed) : m_seed(seed) {}

// ─── Climate (C / T / M) ─────────────────────────────────────────────────────

ClimateSample TerrainGenerator::sampleClimate(int worldX, int worldZ) const {
    const float x = static_cast<float>(worldX);
    const float z = static_cast<float>(worldZ);
    ClimateSample cl;
    cl.continent = saltedOctaveNoise(
        x * CONTINENT_SCALE, z * CONTINENT_SCALE, 4, 2.0f, 0.5f, 101u);
    cl.temperature = saltedOctaveNoise(
        x * TEMPERATURE_SCALE, z * TEMPERATURE_SCALE, 3, 2.0f, 0.5f, 202u);
    cl.moisture = saltedOctaveNoise(
        x * MOISTURE_SCALE, z * MOISTURE_SCALE, 4, 2.0f, 0.5f, 303u);
    return cl;
}

BiomeType TerrainGenerator::biomeFromClimate(const ClimateSample& cl) {
    if (cl.continent < CONTINENT_OCEAN_TH)
        return BiomeType::Ocean;
    if (cl.temperature < TEMP_COLD_TH)
        return BiomeType::Tundra;
    if (cl.moisture >= 0.0f)
        return BiomeType::Forest;
    if (cl.temperature >= TEMP_HOT_TH)
        return BiomeType::Desert;
    return BiomeType::Savanna;
}

TerrainGenerator::BiomeAmps TerrainGenerator::blendedAmps(const ClimateSample& cl) {
    const float B = BIOME_BLEND;
    const float tundraW =
        1.0f - smoothstepRange(TEMP_COLD_TH - B, TEMP_COLD_TH + B, cl.temperature);
    const float hotW =
        smoothstepRange(TEMP_HOT_TH - B, TEMP_HOT_TH + B, cl.temperature);
    const float midW = std::max(0.0f, 1.0f - tundraW - hotW);
    const float wetW = smoothstepRange(-B, B, cl.moisture);
    const float dryW = 1.0f - wetW;

    const float forestW = (1.0f - tundraW) * wetW;
    const float savannaW = midW * dryW;
    const float desertW = hotW * dryW;

    BiomeAmps a;
    a.hill = forestW * HILL_AMP_FOREST
           + tundraW * HILL_AMP_TUNDRA
           + savannaW * HILL_AMP_SAVANNA
           + desertW * HILL_AMP_DESERT;
    a.mount = forestW * MOUNT_AMP_FOREST
            + tundraW * MOUNT_AMP_TUNDRA
            + savannaW * MOUNT_AMP_SAVANNA
            + desertW * MOUNT_AMP_DESERT;
    a.desert = desertW;
    return a;
}

BiomeType TerrainGenerator::getBiome(int worldX, int worldZ) const {
    return biomeFromClimate(sampleClimate(worldX, worldZ));
}

float TerrainGenerator::getMoisture(int worldX, int worldZ) const {
    return sampleClimate(worldX, worldZ).moisture;
}

bool TerrainGenerator::isDeepDesert(int worldX, int worldZ) const {
    const ClimateSample cl = sampleClimate(worldX, worldZ);
    return biomeFromClimate(cl) == BiomeType::Desert
        && cl.moisture < DEEP_DESERT_MOISTURE;
}

TerrainColumn TerrainGenerator::sampleColumn(int worldX, int worldZ) const {
    const ClimateSample cl = sampleClimate(worldX, worldZ);
    TerrainColumn col;
    col.biome = biomeFromClimate(cl);
    col.moisture = cl.moisture;
    col.height = computeHeight(worldX, worldZ, cl);
    return col;
}

// ─── Height ──────────────────────────────────────────────────────────────────

float TerrainGenerator::oceanHeight(float x, float z) const {
    return saltedOctaveNoise(x * 0.020f, z * 0.020f, 3, 2.0f, 0.5f, 10u) * 6.0f
         + static_cast<float>(WATER_LEVEL) - 8.0f;
}

TerrainGenerator::LandRelief TerrainGenerator::sampleLandRelief(float x, float z) const {
    LandRelief r;

    r.plains = saltedOctaveNoise(x * 0.028f, z * 0.028f, 3, 2.0f, 0.50f, 20u) * 5.0f;

    const float hillN =
        saltedOctaveNoise(x * 0.032f, z * 0.032f, 4, 2.0f, 0.55f, 30u);
    const float hillPos = std::max(0.0f, hillN);
    r.hills = hillPos * hillPos * 16.0f;

    const float mountMaskN =
        saltedOctaveNoise(x * 0.0065f, z * 0.0065f, 4, 2.0f, 0.58f, 40u);
    r.mountMask = smoothstepRange(0.22f, 0.58f, mountMaskN);

    const float ridgeBase =
        saltedOctaveNoise(x * 0.011f, z * 0.011f, 5, 2.0f, 0.58f, 50u);
    const float ridge = 1.0f - std::abs(ridgeBase);

    const float peakVar =
        saltedOctaveNoise(x * 0.0035f, z * 0.0035f, 3, 2.0f, 0.50f, 60u);
    const float peakAmp = 18.0f + (peakVar * 0.5f + 0.5f) * 30.0f;

    const float detail =
        saltedOctaveNoise(x * 0.042f, z * 0.042f, 3, 2.0f, 0.45f, 70u) * 7.0f;

    r.mountains = r.mountMask * (ridge * peakAmp + detail);
    return r;
}

float TerrainGenerator::riverMask(float x, float z, float landW, float mountMask,
                                  float mountAmp, float moisture) const {
    const float warpX =
        saltedOctaveNoise(x * RIVER_WARP_SCALE, z * RIVER_WARP_SCALE, 3, 2.0f, 0.5f, 405u)
        * RIVER_WARP_AMP;
    const float warpZ =
        saltedOctaveNoise(x * RIVER_WARP_SCALE, z * RIVER_WARP_SCALE, 3, 2.0f, 0.5f, 406u)
        * RIVER_WARP_AMP;

    const float n = saltedOctaveNoise(
        (x + warpX) * RIVER_SCALE, (z + warpZ) * RIVER_SCALE, 4, 2.0f, 0.5f, 404u);
    const float ridge = 1.0f - std::abs(n);

    const float m01 = std::clamp((moisture + 1.0f) * 0.5f, 0.0f, 1.0f);
    // Wet: denser / slightly wider (lower ridge threshold). Dry: almost none.
    const float lo = lerp(0.955f, RIVER_RIDGE_LO - 0.02f, m01);
    const float hi = lerp(0.990f, RIVER_RIDGE_HI, m01);
    const float ridgeMask = smoothstepRange(lo, hi, ridge);

    float moistureWiden = m01;
    if (m01 < 0.18f)
        moistureWiden *= m01 / 0.18f;

    const float peakBlock = 1.0f - mountMask * mountAmp;
    return ridgeMask * landW * peakBlock * moistureWiden;
}

int TerrainGenerator::computeHeight(int worldX, int worldZ,
                                    const ClimateSample& cl) const {
    const float x = static_cast<float>(worldX);
    const float z = static_cast<float>(worldZ);
    const float B = BIOME_BLEND;
    const float landW = smoothstepRange(
        CONTINENT_OCEAN_TH - B, CONTINENT_OCEAN_TH + B, cl.continent);

    const LandRelief relief = sampleLandRelief(x, z);
    const BiomeAmps amps = blendedAmps(cl);

    const float dunes =
        saltedOctaveNoise(x * 0.040f, z * 0.040f, 2, 2.0f, 0.4f, 80u) * DUNE_HEIGHT;

    const float landH = static_cast<float>(WATER_LEVEL) + 2.0f
                      + relief.plains
                      + relief.hills * amps.hill
                      + relief.mountains * amps.mount
                      + amps.desert * dunes;

    float h;
    if (landW <= 0.0f)
        h = oceanHeight(x, z);
    else if (landW >= 1.0f)
        h = landH;
    else
        h = lerp(oceanHeight(x, z), landH, landW);

    const float rmask = riverMask(x, z, landW, relief.mountMask, amps.mount, cl.moisture);
    const float target = static_cast<float>(WATER_LEVEL - 1);
    if (h > target)
        h = lerp(h, target, rmask);

    return std::clamp(static_cast<int>(h), 2, WORLD_HEIGHT - 16);
}

int TerrainGenerator::getHeight(int worldX, int worldZ) const {
    return computeHeight(worldX, worldZ, sampleClimate(worldX, worldZ));
}

// ─── Block (surface follows climate; no forced beaches on grass) ─────────────

int TerrainGenerator::getBlock(int worldX, int y, int worldZ) const {
    const TerrainColumn col = sampleColumn(worldX, worldZ);
    return getBlock(worldX, y, worldZ, col.height, col.biome);
}

int TerrainGenerator::getBlock(int worldX, int y, int worldZ,
                               int height, BiomeType biome) const {
    (void)worldX;
    (void)worldZ;

    if (y > height) {
        if (y <= WATER_LEVEL) {
            if (biome == BiomeType::Tundra && y == WATER_LEVEL)
                return static_cast<int>(BlockId::Ice);
            return static_cast<int>(BlockId::Water);
        }
        return static_cast<int>(BlockId::Air);
    }

    if (y == height) {
        switch (biome) {
            case BiomeType::Ocean:
            case BiomeType::Desert:
                return static_cast<int>(BlockId::Sand);
            case BiomeType::Forest:
                return static_cast<int>(BlockId::Grass);
            case BiomeType::Savanna:
                return static_cast<int>(BlockId::SavannaGrass);
            case BiomeType::Tundra:
                return static_cast<int>(BlockId::Snow);
            default:
                return static_cast<int>(BlockId::Grass);
        }
    }

    // height-3 .. height-1: sand (ocean/desert) or dirt
    if (y >= height - SUBSOIL_LAYERS) {
        switch (biome) {
            case BiomeType::Desert:
            case BiomeType::Ocean:
                return static_cast<int>(BlockId::Sand);
            default:
                return static_cast<int>(BlockId::Dirt);
        }
    }

    // height-11 .. height-4: sandstone in desert, stone elsewhere
    if (biome == BiomeType::Desert
        && y >= height - SUBSOIL_LAYERS - SANDSTONE_LAYERS) {
        return static_cast<int>(BlockId::Sandstone);
    }

    return static_cast<int>(BlockId::Stone);
}

// ─── Tree placement ──────────────────────────────────────────────────────────

bool TerrainGenerator::shouldPlaceTree(int worldX, int worldZ) const {
    return shouldPlaceTree(worldX, worldZ, sampleColumn(worldX, worldZ));
}

bool TerrainGenerator::shouldPlaceTree(int worldX, int worldZ,
                                      const TerrainColumn& col) const {
    if (col.height <= WATER_LEVEL)
        return false;

    const int surface = getBlock(worldX, col.height, worldZ, col.height, col.biome);
    float base = 0.0f;
    if (col.biome == BiomeType::Forest && surface == static_cast<int>(BlockId::Grass))
        base = 1.0f / 18.0f;
    else if (col.biome == BiomeType::Savanna
             && surface == static_cast<int>(BlockId::SavannaGrass))
        base = 1.0f / 90.0f;
    else
        return false;

    if (col.biome == BiomeType::Forest && col.height > WATER_LEVEL + 18)
        base *= 0.45f;

    return columnRoll(worldX, worldZ, 501u) < base * wetFactor(col.moisture);
}

int TerrainGenerator::getTreeHeight(int worldX, int worldZ) const {
    return getTreeHeight(worldX, worldZ, getBiome(worldX, worldZ));
}

int TerrainGenerator::getTreeHeight(int worldX, int worldZ, BiomeType biome) const {
    const float u = columnRoll(worldX + 3000, worldZ + 7000, 502u);
    if (biome == BiomeType::Savanna) {
        int h = 3 + static_cast<int>(u * 3.0f);
        return std::min(h, 5);
    }
    int h = 4 + static_cast<int>(u * 3.0f);
    return std::min(h, 6);
}
