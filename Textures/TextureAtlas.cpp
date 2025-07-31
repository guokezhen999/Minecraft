//
// Created by 郭珂桢 on 2024/5/24.
//

#include "TextureAtlas.h"

TextureAtlas::TextureAtlas(const std::string &textureFilename) : BasicTexture(textureFilename)
{
    std::string file = "resources/textures" + textureFilename + ".png";
    LoadFromFile(file.c_str());
    m_imageSize = 256;
    m_individualTextureSize = 16;
}

std::array<float, 8> TextureAtlas::GetTexture(const glm::ivec2 &coords) const
{
    static const float TEX_PER_ROW = (float)m_imageSize / (float)m_individualTextureSize;
    static const float INDV_TEX_SIZE = 1.0f / TEX_PER_ROW;
    static const float PIXEL_SIZE = 1.0f / (float)m_imageSize;

    float xMin = ((float)coords.x * INDV_TEX_SIZE) + 0.5f * PIXEL_SIZE;
    float yMin = ((float)coords.y * INDV_TEX_SIZE) + 0.5f * PIXEL_SIZE;

    float xMax = (xMin + INDV_TEX_SIZE) - PIXEL_SIZE;
    float yMax = (yMin + INDV_TEX_SIZE) - PIXEL_SIZE;

    return {xMax, yMax, xMin, yMax, xMin, yMin, xMax, yMin};
}
