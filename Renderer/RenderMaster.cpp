//
// Created by 郭珂桢 on 2024/5/23.
//

#include "RenderMaster.h"
#include "../ResourceManager.h"

void RenderMaster::ensureChunkRenderer() {
    if (!m_chunkRenderer) {
        m_chunkRenderer = std::make_unique<ChunkRenderer>();
    }
}

void RenderMaster::InitCubeRenderer()
{
    m_CubeRenderers = new CubeRenderer[(int)BlockId::NUM_TYPES];
    for (int i = 0; i < (int)BlockId::NUM_TYPES; i++)
    {
        m_CubeRenderers[i] = CubeRenderer(*ResourceManager::BlockShader, (BlockId)i);
    }
}

void RenderMaster::RenderBlocks(Camera &camera)
{
    glm::vec3 positions[] = {
            {0.0, 0.0, 0.0},
            {1.0, 2.0, 2.0},
            {2.0, 3.0, 3.0},
            {3.0, 4.0, 4.0},
            {5.0, 4.5, 4.0}
    };
    m_CubeRenderers[1].Render(&camera, positions[0]);
    m_CubeRenderers[2].Render(&camera, positions[1]);
    m_CubeRenderers[10].Render(&camera, positions[2]);
    m_CubeRenderers[8].Render(&camera, positions[3]);
    m_CubeRenderers[9].Render(&camera, positions[4]);
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

void RenderMaster::FinishChunkRender(const Camera &camera, bool underwater)
{
    ensureChunkRenderer();
    m_chunkRenderer->Render(camera, underwater);
}
