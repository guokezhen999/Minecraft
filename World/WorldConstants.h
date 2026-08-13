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

// Climate map frequency (higher = smaller biomes).
// ~0.008 → patches roughly 100+ blocks across.
constexpr float BIOME_SCALE = 0.008f;

// Half-width of ocean↔land height blend in climate-noise space.
constexpr float BIOME_BLEND = 0.08f;

// Chunk streaming
constexpr int RENDER_DISTANCE = 10;
constexpr int UNLOAD_DISTANCE = RENDER_DISTANCE + 2;

// Main-thread GPU uploads per frame (CPU gen/mesh run on worker)
constexpr int MAX_MESH_UPLOADS_PER_FRAME = 6;
constexpr int MAX_CHUNKS_INTEGRATE_PER_FRAME = 3;

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
constexpr float NIGHT_DAY_FACTOR = 0.05f;

struct Atmosphere {
    float dayFactor = 1.0f;
    glm::vec3 skyTop{SKY_TOP_R, SKY_TOP_G, SKY_TOP_B};
    glm::vec3 skyHorizon{SKY_HORIZON_R, SKY_HORIZON_G, SKY_HORIZON_B};
    glm::vec3 fogColor{FOG_R, FOG_G, FOG_B};
};

#endif //MINECRAFT_WORLDCONSTANTS_H
