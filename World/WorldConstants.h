//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_WORLDCONSTANTS_H
#define MINECRAFT_WORLDCONSTANTS_H

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
constexpr int MAX_MESH_UPLOADS_PER_FRAME = 8;

// LOD: skip flora beyond this world distance (blocks)
constexpr float FLORA_LOD_DISTANCE = static_cast<float>(RENDER_DISTANCE * CHUNK_SIZE) * 0.55f;

// Block interaction
constexpr float RAYCAST_REACH = 6.0f;

#endif //MINECRAFT_WORLDCONSTANTS_H
