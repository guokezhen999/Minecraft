//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_RENDERMASTER_H
#define MINECRAFT_RENDERMASTER_H

#include "CubeRenderer.h"
#include "ChunkRenderer.h"
#include "../World/Chunk/ChunkMesh.h"

class RenderMaster
{
public:
    CubeRenderer *m_CubeRenderers;
    bool m_DrawBox = false;

    void RenderBlocks(Camera &camera);

    // Queue chunk meshes; flora optional (LOD)
    void DrawChunk(const ChunkMeshCollection& meshes, float distSq, bool drawFlora);

    void FinishChunkRender(const Camera &camera, bool underwater = false);

    void InitCubeRenderer();

private:
    std::unique_ptr<ChunkRenderer> m_chunkRenderer;
    void ensureChunkRenderer();
};


#endif //MINECRAFT_RENDERMASTER_H
