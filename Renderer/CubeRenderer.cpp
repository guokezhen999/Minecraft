//
// Created by 郭珂桢 on 2024/5/20.
//

#include "CubeRenderer.h"
#include "../ResourceManager.h"
#include "../World/Block/BlockDataBase.h"

#include <iostream>

CubeRenderer::CubeRenderer(BasicShader &shader, BlockId id)
{
    this->m_Shader = shader;
    this->initRenderData(id);
}

void CubeRenderer::Render(Camera* camera)
{
    this->m_Shader.Use();
    glm::mat4 model(1.0f);
    glm::mat4 projectionView = camera->GetProjectionViewMatrix();

    m_Shader.SetMatrix4f("model", model);
    m_Shader.SetMatrix4f("projectionView", projectionView);

    glActiveTexture(GL_TEXTURE0);
    ResourceManager::BlockTexture.Bind();

    glBindVertexArray(this->cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void CubeRenderer::Render(Camera *camera, glm::vec3 position)
{
    this->m_Shader.Use();
    glm::mat4 projectionView = camera->GetProjectionViewMatrix();
    glm::mat4 model(1.0);
    model = glm::translate(model, position);

    m_Shader.SetMatrix4f("model", model);
    m_Shader.SetMatrix4f("projectionView", projectionView);

    glActiveTexture(GL_TEXTURE0);
    ResourceManager::BlockTexture.Bind();

    glBindVertexArray(this->cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


void CubeRenderer::initRenderData(BlockId id)
{
    unsigned int VBO;
    glm::ivec2 sideCoords = BlockDatabase::Get().GetData(id).GetBlockData().texSideCoords;
    glm::ivec2 bottomCoords = BlockDatabase::Get().GetData(id).GetBlockData().texBottomCoords;
    glm::ivec2 topCoords = BlockDatabase::Get().GetData(id).GetBlockData().texTopCoords;

    float side_x_min = (float)sideCoords.x / 16.0f, side_x_max = ((float)sideCoords.x + 1) / 16.0f;
    float side_y_min = (float)sideCoords.y / 16.0f, side_y_max = ((float)sideCoords.y + 1) / 16.0f;
    float bottom_x_min = (float)bottomCoords.x / 16.0f, bottom_x_max = ((float)bottomCoords.x + 1) / 16.0f;
    float bottom_y_min = (float)bottomCoords.y / 16.0f, bottom_y_max = ((float)bottomCoords.y + 1) / 16.0f;
    float top_x_min = (float)topCoords.x / 16.0f, top_x_max = ((float)topCoords.x + 1) / 16.0f;
    float top_y_min = (float)topCoords.y / 16.0f, top_y_max = ((float)topCoords.y + 1) / 16.0f;

    std::cout << side_x_min << " " << side_y_min << " " << bottom_x_min << " " << bottom_x_min << " "
    << " " << top_x_min << " " << top_y_min << std::endl;

    float cubeVertices[] = {
            // positions          // Textures Coords
            -0.5f, -0.5f, -0.5f, side_x_min,  side_y_max,
            0.5f, -0.5f, -0.5f, side_x_max, side_y_max,
            0.5f,  0.5f, -0.5f, side_x_max, side_y_min,
            0.5f,  0.5f, -0.5f, side_x_max, side_y_min,
            -0.5f,  0.5f, -0.5f, side_x_min,  side_y_min,
            -0.5f, -0.5f, -0.5f, side_x_min,  side_y_max,

            -0.5f, -0.5f,  0.5f, side_x_min, side_y_max,
            0.5f, -0.5f,  0.5f, side_x_max, side_y_max,
            0.5f,  0.5f,  0.5f, side_x_max, side_y_min,
            0.5f,  0.5f,  0.5f, side_x_max, side_y_min,
            -0.5f,  0.5f,  0.5f, side_x_min, side_y_min,
            -0.5f, -0.5f,  0.5f, side_x_min, side_y_max,

            -0.5f,  0.5f,  0.5f, side_x_min, side_y_min,
            -0.5f,  0.5f, -0.5f, side_x_max, side_y_min,
            -0.5f, -0.5f, -0.5f, side_x_max, side_y_max,
            -0.5f, -0.5f, -0.5f, side_x_max, side_y_max,
            -0.5f, -0.5f,  0.5f, side_x_min, side_y_max,
            -0.5f,  0.5f,  0.5f, side_x_min, side_y_min,

            0.5f,  0.5f,  0.5f, side_x_min, side_y_min,
            0.5f,  0.5f, -0.5f, side_x_max, side_y_min,
            0.5f, -0.5f, -0.5f, side_x_max, side_y_max,
            0.5f, -0.5f, -0.5f, side_x_max, side_y_max,
            0.5f, -0.5f,  0.5f, side_x_min, side_y_max,
            0.5f,  0.5f,  0.5f, side_x_min, side_y_min,

            -0.5f, -0.5f, -0.5f,  bottom_x_min, bottom_y_min,
            0.5f, -0.5f, -0.5f,  bottom_x_max, bottom_y_min,
            0.5f, -0.5f,  0.5f,  bottom_x_max, bottom_y_max,
            0.5f, -0.5f,  0.5f,  bottom_x_max, bottom_y_max,
            -0.5f, -0.5f,  0.5f,  bottom_x_min, bottom_y_max,
            -0.5f, -0.5f, -0.5f,  bottom_x_min, bottom_y_min,

            -0.5f,  0.5f, -0.5f,  top_x_min, top_y_min,
            0.5f,  0.5f, -0.5f,  top_x_max, top_y_min,
            0.5f,  0.5f,  0.5f,  top_x_max, top_y_max,
            0.5f,  0.5f,  0.5f,  top_x_max, top_y_max,
            -0.5f,  0.5f,  0.5f,  top_x_min, top_y_max,
            -0.5f,  0.5f, -0.5f,  top_x_min, top_y_min
    };

    glGenVertexArrays(1, &this->cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);

    glBindVertexArray(this->cubeVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false,
                          5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false,
                          5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}



