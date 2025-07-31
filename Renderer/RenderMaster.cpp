//
// Created by 郭珂桢 on 2024/5/23.
//

#include "RenderMaster.h"
#include "../ResourceManager.h"

void RenderMaster::InitCubeRenderer()
{
    m_CubeRenderers = new CubeRenderer[(int)BlockId::NUM_TYPES];
    for (int i = 0; i < (int)BlockId::NUM_TYPES; i++)
    {
        m_CubeRenderers[i] = CubeRenderer(ResourceManager::BlockShader, (BlockId)i);
    }
}

void RenderMaster::RenderBlocks(Camera &camera)
{
    glm::vec3 positions[] = {
            {0.0, 0.0, 0.0},
            {1.0, 2.0, 2.0},
            {2.0, 3.0, 3.0}
    };
    m_CubeRenderers[1].Render(&camera, positions[0]);
    m_CubeRenderers[2].Render(&camera, positions[1]);
    m_CubeRenderers[3].Render(&camera, positions[2]);

}

