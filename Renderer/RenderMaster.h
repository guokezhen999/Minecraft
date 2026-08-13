//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_RENDERMASTER_H
#define MINECRAFT_RENDERMASTER_H

#include "ChunkRenderer.h"
#include "../World/Chunk/ChunkMesh.h"
#include "../World/WorldConstants.h"

#include <memory>

class RenderMaster
{
public:
    // Queue chunk meshes; flora optional (LOD)
    void DrawChunk(const ChunkMeshCollection& meshes, float distSq, bool drawFlora);

    void FinishChunkRender(const Camera &camera, bool underwater,
                           const Atmosphere& atmosphere);

    void shutdown();

private:
    std::unique_ptr<ChunkRenderer> m_chunkRenderer;
    void ensureChunkRenderer();
};


#endif //MINECRAFT_RENDERMASTER_H
