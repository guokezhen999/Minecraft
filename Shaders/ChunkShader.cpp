//
// Created by 郭珂桢 on 2024/5/29.
//

#include "ChunkShader.h"

ChunkShader::ChunkShader()
: BasicShader("resources/shaders/chunk.vert",
              "resources/shaders/chunk.frag")
{
    GetUniforms();
}

void ChunkShader::GetUniforms()
{
    BasicShader::GetUniforms();
}
