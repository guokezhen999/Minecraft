//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_RENDERMASTER_H
#define MINECRAFT_RENDERMASTER_H

#include "CubeRenderer.h"
#include "ChunkRenderer.h"

class ChunkSection;

class RenderMaster
{
public:
    CubeRenderer *m_CubeRenderers;
    bool m_DrawBox = false;

    void RenderBlocks(Camera &camera);
    void DrawChunk(Camera &camera);
    void InitCubeRenderer();

private:
    ChunkRenderer *m_chunkRenderer;
};


#endif //MINECRAFT_RENDERMASTER_H
