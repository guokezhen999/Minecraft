//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_BASICSHADER_H
#define MINECRAFT_BASICSHADER_H

#include "Shader.h"

class BasicShader : public Shader
{
public:
    BasicShader() = default;
    BasicShader(const std::string &vertexFile,
                const std::string &fragmentFile);
    void SetProjectionView(const glm::mat4 &matrix);
    void SetModel(const glm::mat4 &matrix);

    virtual void GetUniforms() override;

private:
    GLuint m_locationProjectionViewMatrix{};
    GLuint m_locationModelMatrix{};
};


#endif //MINECRAFT_BASICSHADER_H
