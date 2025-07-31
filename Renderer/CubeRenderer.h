//
// Created by 郭珂桢 on 2024/5/20.
//

#ifndef MINECRAFT_CUBERENDERER_H
#define MINECRAFT_CUBERENDERER_H

#include "../Textures/BasicTexture.h"
#include "../Shaders/BasicShader.h"
#include "../Camera.h"
#include "../World/Block/BlockId.h"

#include <map>

class CubeRenderer
{
public:
    CubeRenderer() = default;
    CubeRenderer(BasicShader &shader, BlockId id);

    void Render(Camera* camera);
    void Render(Camera* camera, glm::vec3 position);
    BasicShader m_Shader;

private:
    GLuint cubeVAO = 0;
    void initRenderData(BlockId id);
};


#endif //MINECRAFT_CUBERENDERER_H
