//
// Created by 郭珂桢 on 2024/5/29.
//

#ifndef MINECRAFT_CHUNKRENDERER_H
#define MINECRAFT_CHUNKRENDERER_H

#include <vector>

#include "../Shaders/ChunkShader.h"

struct RenderInfo;
class ChunkMesh;
class Camera;

class ChunkRenderer
{
public:
    void Add(const ChunkMesh &mesh);
    void Render(const Camera &camera);

private:
    std::vector<const RenderInfo *> m_chunks;

    ChunkShader m_shader;
};


#endif //MINECRAFT_CHUNKRENDERER_H
