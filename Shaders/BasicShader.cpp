//
// Created by 郭珂桢 on 2024/5/23.
//

#include "BasicShader.h"

BasicShader::BasicShader(const std::string &vertexFile, const std::string &fragmentFile)
        : Shader(vertexFile, fragmentFile)
{
    this->Use();
}

void BasicShader::SetModel(const glm::mat4 &matrix)
{
    SetMatrix4f(this->m_locationModelMatrix, matrix);
}

void BasicShader::SetProjectionView(const glm::mat4 &matrix)
{
    SetMatrix4f(this->m_locationProjectionViewMatrix, matrix);
}

void BasicShader::GetUniforms()
{
    this->Use();
    m_locationProjectionViewMatrix = glGetUniformLocation(this->ID, "projectionView");
    m_locationModelMatrix = glGetUniformLocation(this->ID, "model");
}
