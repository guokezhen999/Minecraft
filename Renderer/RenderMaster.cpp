//
// Created by 郭珂桢 on 2024/5/23.
//

#include "RenderMaster.h"

void RenderMaster::ensureChunkRenderer() {
    if (!m_chunkRenderer) {
        m_chunkRenderer = std::make_unique<ChunkRenderer>();
    }
}

void RenderMaster::DrawChunk(const ChunkMeshCollection& meshes, float distSq, bool drawFlora)
{
    ensureChunkRenderer();
    m_chunkRenderer->AddSolid(meshes.solidMesh);
    m_chunkRenderer->AddWater(meshes.waterMesh, distSq);
    if (drawFlora) {
        m_chunkRenderer->AddTransparent(meshes.floraMesh, distSq);
    }
}

void RenderMaster::FinishChunkRender(const Camera &camera, bool underwater,
                                     const Atmosphere& atmosphere)
{
    ensureChunkRenderer();
    m_chunkRenderer->Render(camera, underwater, atmosphere);
}

void RenderMaster::shutdown()
{
    m_chunkRenderer.reset();
}
