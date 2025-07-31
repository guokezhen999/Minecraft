//
// Created by 郭珂桢 on 2024/5/24.
//

#ifndef MINECRAFT_TEXTUREATLAS_H
#define MINECRAFT_TEXTUREATLAS_H

#include "BasicTexture.h"

#include "glm/glm.hpp"

#include <array>

class TextureAtlas : public BasicTexture
{
public:
    TextureAtlas() = default;
    TextureAtlas(const std::string &textureFilename);

    std::array<float, 8> GetTexture(const glm::ivec2 &coords) const;

private:
    int m_imageSize;
    int m_individualTextureSize;
};


#endif //MINECRAFT_TEXTUREATLAS_H
