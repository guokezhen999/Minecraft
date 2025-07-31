//
// Created by 郭珂桢 on 2024/5/26.
//

#ifndef MINECRAFT_RESOURCEMANAGER_H
#define MINECRAFT_RESOURCEMANAGER_H

#include "../Util/Singleton.h"
#include "Shaders/BasicShader.h"
#include "Textures/TextureAtlas.h"
#include "World/Block/BlockTypes/BlockType.h"
#include "World/Block/BlockId.h"

#include <array>

class ResourceManager
{
public:
    static BasicShader BlockShader;
    static BasicTexture BlockTexture;

    static BasicTexture LoadTextureFromFile(const GLchar *file);

};

#endif //MINECRAFT_RESOURCEMANAGER_H
