//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_WORLDCONSTANTS_H
#define MINECRAFT_WORLDCONSTANTS_H

constexpr int CHUNK_SIZE = 16, CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE,
    CHUNK_VOLUME = CHUNK_SIZE * CHUNK_AREA,
    // Single-layer world: keep water within [0, CHUNK_SIZE)
    WATER_LEVEL = 8;

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
