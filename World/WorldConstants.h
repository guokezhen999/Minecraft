//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_WORLDCONSTANTS_H
#define MINECRAFT_WORLDCONSTANTS_H

#include <glm/glm.hpp>

constexpr int CHUNK_SIZE = 16, CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE,
    CHUNK_VOLUME = CHUNK_SIZE * CHUNK_AREA;

// Vertical world: 8 sections × 16 = 128
constexpr int WORLD_HEIGHT = 128;
constexpr int CHUNK_SECTIONS = WORLD_HEIGHT / CHUNK_SIZE; // 8

// Sea level for the taller world (roughly Minecraft-like proportion)
constexpr int WATER_LEVEL = 62;

// Climate: three uncorrelated 2D fields, each ~[-1, 1].
constexpr float CONTINENT_SCALE = 0.004f;    // C — ocean vs land
// Lower = larger hot/temperate/cold patches (~1/scale blocks across).
constexpr float TEMPERATURE_SCALE = 0.003f;  // T — hot / temperate / cold
constexpr float MOISTURE_SCALE = 0.004f;     // M — dry / wet, rivers, plants

// Ocean when C < this; land uses (T, M) for the 9 biomes.
constexpr float CONTINENT_OCEAN_TH = -0.28f;

// Four temperature bands: arctic / subarctic / temperate / tropical.
constexpr float TEMP_ARCTIC_TH = -0.6f;
constexpr float TEMP_SUBARCTIC_TH = -0.2f;
constexpr float TEMP_TROPICAL_TH = 0.3f;

// Moisture splits inside each temperature band (see biome matrix).
constexpr float MOIST_TROP_DRY_TH = -0.2f;   // Desert | Savanna
constexpr float MOIST_TROP_WET_TH = 0.4f;    // Savanna | Jungle
constexpr float MOIST_TEMP_DRY_TH = -0.4f;   // TemperateDesert | Grassland
constexpr float MOIST_TEMP_WET_TH = 0.2f;    // Grassland | Forest
constexpr float MOIST_SUBARCTIC_TH = 0.0f;   // Tundra | Taiga

// Half-width of climate blends (height amps, ocean↔land height).
constexpr float BIOME_BLEND = 0.08f;

// Relief multipliers after the shared plains / hills / mountains stack.
constexpr float HILL_AMP_SNOWY_PLAINS = 0.5f;
constexpr float MOUNT_AMP_SNOWY_PLAINS = 0.2f;

constexpr float HILL_AMP_TUNDRA = 0.6f;
constexpr float MOUNT_AMP_TUNDRA = 0.3f;

constexpr float HILL_AMP_TAIGA = 0.9f;
constexpr float MOUNT_AMP_TAIGA = 0.8f;

constexpr float HILL_AMP_TEMP_DESERT = 0.4f;
constexpr float MOUNT_AMP_TEMP_DESERT = 0.1f;

constexpr float HILL_AMP_GRASSLAND = 0.5f;
constexpr float MOUNT_AMP_GRASSLAND = 0.2f;

constexpr float HILL_AMP_FOREST = 1.0f;
constexpr float MOUNT_AMP_FOREST = 1.0f;

constexpr float HILL_AMP_DESERT = 0.35f;
constexpr float MOUNT_AMP_DESERT = 0.0f;

constexpr float HILL_AMP_SAVANNA = 0.6f;
constexpr float MOUNT_AMP_SAVANNA = 0.15f;

constexpr float HILL_AMP_JUNGLE = 1.2f;
constexpr float MOUNT_AMP_JUNGLE = 0.9f;

constexpr float DUNE_HEIGHT = 2.5f;

// Column fill (below the surface block).
constexpr int SUBSOIL_LAYERS = 3;      // height-3 .. height-1
constexpr int SANDSTONE_LAYERS = 8;    // desert: height-11 .. height-4

// Rivers (ridge of salted octave noise, carved down only).
constexpr float RIVER_SCALE = 0.0035f;
constexpr float RIVER_WARP_SCALE = 0.012f;
constexpr float RIVER_WARP_AMP = 28.0f;
constexpr float RIVER_RIDGE_LO = 0.90f;
constexpr float RIVER_RIDGE_HI = 0.97f;

// Desert interiors (no cactus / dead shrub).
constexpr float DEEP_DESERT_MOISTURE = -0.50f;

// Chunk streaming
constexpr int RENDER_DISTANCE = 10;
constexpr int UNLOAD_DISTANCE = RENDER_DISTANCE + 2;
// First load: 5×5 around the camera, then grow to render distance
constexpr int STREAM_START_RADIUS = 2;

// Main-thread GPU uploads per frame (CPU gen/mesh run on workers)
constexpr int MAX_MESH_UPLOADS_PER_FRAME = 6;
constexpr int MAX_CHUNKS_INTEGRATE_PER_FRAME = 3;
// Higher budgets while the view is still filling in
constexpr int MAX_MESH_UPLOADS_STREAMING = 12;
constexpr int MAX_CHUNKS_INTEGRATE_STREAMING = 12;

// LOD: skip flora beyond this world distance (blocks)
constexpr float FLORA_LOD_DISTANCE = static_cast<float>(RENDER_DISTANCE * CHUNK_SIZE) * 0.55f;

// Block interaction
constexpr float RAYCAST_REACH = 6.0f;

// Fluid simulation (main thread)
constexpr int MAX_FLUID_UPDATES_PER_FRAME = 192;

// Fog (world-space distance from camera; covers chunk load edge)
constexpr float FOG_START = static_cast<float>(RENDER_DISTANCE * CHUNK_SIZE) * 0.55f;
constexpr float FOG_END   = static_cast<float>(RENDER_DISTANCE * CHUNK_SIZE) * 0.92f;

// Daytime sky / fog tint (RGB 0–1)
constexpr float SKY_TOP_R = 0.37f, SKY_TOP_G = 0.58f, SKY_TOP_B = 0.98f;
constexpr float SKY_HORIZON_R = 0.72f, SKY_HORIZON_G = 0.82f, SKY_HORIZON_B = 0.95f;
constexpr float FOG_R = SKY_HORIZON_R, FOG_G = SKY_HORIZON_G, FOG_B = SKY_HORIZON_B;

// Underwater look (dense blue-green fog)
constexpr float UNDERWATER_FOG_R = 0.04f, UNDERWATER_FOG_G = 0.18f, UNDERWATER_FOG_B = 0.32f;
constexpr float UNDERWATER_FOG_START = 4.0f;
constexpr float UNDERWATER_FOG_END = 22.0f;
constexpr float UNDERWATER_TINT_R = 0.15f, UNDERWATER_TINT_G = 0.40f, UNDERWATER_TINT_B = 0.55f;

// Lighting / day cycle
constexpr int LIGHT_LEVEL_MAX = 15;
constexpr int LIGHT_EDIT_RADIUS = 16;
constexpr int DAY_LENGTH = 24000;
constexpr int TICKS_PER_SECOND = 20;
constexpr int TICK_NOON = 6000;
constexpr int TICK_MIDNIGHT = 18000;
// Half-width of full day / full night. Leftover ticks are dawn / dusk (~30s each).
constexpr int DAY_PLATEAU_HALF = 4800;
constexpr int NIGHT_PLATEAU_HALF = 4800;
// ~5 min per full cycle (Minecraft is 20).
constexpr float WORLD_TIME_SCALE = 4.0f;
// Hold T to speed up the day cycle (1 = no extra, 5 = five times faster).
constexpr float TIME_FAST_FORWARD = 10.0f;
// Moonlight scale on sky light (night is dimmer than day, not black).
constexpr float NIGHT_DAY_FACTOR = 0.15f;
// Direct sun vs sky fill. Keep sun stronger so east/west/south walls read clearly.
constexpr float SUN_STRENGTH = 0.88f;
constexpr float MOON_STRENGTH = 0.22f;
constexpr float SKY_GI_STRENGTH = 0.40f;
// Noon sun sits in the southern sky (~50°), not at zenith — otherwise all walls look the same.

struct Atmosphere {
    float dayFactor = 1.0f;
    glm::vec3 skyTop{SKY_TOP_R, SKY_TOP_G, SKY_TOP_B};
    glm::vec3 skyHorizon{SKY_HORIZON_R, SKY_HORIZON_G, SKY_HORIZON_B};
    glm::vec3 fogColor{FOG_R, FOG_G, FOG_B};
    float fogStart = FOG_START;
    float fogEnd = FOG_END;
    glm::vec3 sunDir{0.0f, 0.75f, 0.66f};
    glm::vec3 sunColor{SUN_STRENGTH, SUN_STRENGTH * 0.96f, SUN_STRENGTH * 0.90f};
    glm::vec3 skyLightColor{SKY_GI_STRENGTH, SKY_GI_STRENGTH, SKY_GI_STRENGTH};
    glm::vec3 sunDiscColor{1.0f, 0.96f, 0.88f};
    glm::vec3 moonDir{0.0f, -1.0f, 0.0f};
    glm::vec3 moonColor{0.0f};
    glm::vec3 moonDiscColor{0.0f};
    bool celestial = true;
};

#endif //MINECRAFT_WORLDCONSTANTS_H
