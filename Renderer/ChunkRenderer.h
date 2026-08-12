//
// Created by 郭珂桢 on 2024/5/29.
//

#ifndef MINECRAFT_CHUNKRENDERER_H
#define MINECRAFT_CHUNKRENDERER_H

#include <vector>
#include <utility>

#include "../Shaders/ChunkShader.h"

class Model;
class ChunkMesh;
class Camera;

class ChunkRenderer
{
public:
    void AddSolid(const ChunkMesh &mesh);
    void AddTransparent(const ChunkMesh &mesh, float distSq);

    // Opaque pass, then back-to-front transparent pass
    void Render(const Camera &camera);

private:
    struct TransparentDraw {
        const Model *model;
        float distSq;
    };

    std::vector<const Model *> m_solid;
    std::vector<TransparentDraw> m_transparent;

    ChunkShader m_shader;
};


#endif //MINECRAFT_CHUNKRENDERER_H
