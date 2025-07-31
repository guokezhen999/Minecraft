//
// Created by 郭珂桢 on 2024/5/29.
//

#ifndef MINECRAFT_CHUNKSHADER_H
#define MINECRAFT_CHUNKSHADER_H

#include "BasicShader.h"

class ChunkShader : public BasicShader
{
public:
    ChunkShader();

private:
    void GetUniforms() override;
};


#endif //MINECRAFT_CHUNKSHADER_H
