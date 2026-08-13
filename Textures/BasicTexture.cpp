//
// Created by 郭珂桢 on 2024/5/20.
//

#include "BasicTexture.h"

#include <iostream>
#include <vector>

#include <glad/glad.h>

#include "stb_image/stb_image.h"

namespace {

// Downsample each atlas tile on its own so mipmaps do not mix neighboring
// tiles (water sits next to sand/cactus; a shared mip texel is green).
void uploadIsolatedAtlasMips(int size, int tileSize, const unsigned char* level0)
{
    const int tiles = size / tileSize;
    int srcW = size;
    int srcTile = tileSize;
    std::vector<unsigned char> src(level0, level0 + size * size * 4);

    int maxLevel = 0;
    for (int dim = size; (dim >> 1) >= tiles; dim >>= 1)
        ++maxLevel;

    std::vector<unsigned char> dst;
    for (int level = 1; level <= maxLevel; ++level) {
        const int dstW = srcW / 2;
        const int dstTile = srcTile / 2;
        dst.assign(static_cast<size_t>(dstW * dstW * 4), 0);

        for (int ty = 0; ty < tiles; ++ty) {
            for (int tx = 0; tx < tiles; ++tx) {
                for (int py = 0; py < dstTile; ++py) {
                    for (int px = 0; px < dstTile; ++px) {
                        int acc[4] = {0, 0, 0, 0};
                        const int sx0 = tx * srcTile + px * 2;
                        const int sy0 = ty * srcTile + py * 2;
                        for (int oy = 0; oy < 2; ++oy) {
                            for (int ox = 0; ox < 2; ++ox) {
                                const unsigned char* p =
                                    src.data() + ((sy0 + oy) * srcW + (sx0 + ox)) * 4;
                                acc[0] += p[0];
                                acc[1] += p[1];
                                acc[2] += p[2];
                                acc[3] += p[3];
                            }
                        }
                        unsigned char* d = dst.data() +
                            ((ty * dstTile + py) * dstW + (tx * dstTile + px)) * 4;
                        d[0] = static_cast<unsigned char>(acc[0] / 4);
                        d[1] = static_cast<unsigned char>(acc[1] / 4);
                        d[2] = static_cast<unsigned char>(acc[2] / 4);
                        d[3] = static_cast<unsigned char>(acc[3] / 4);
                    }
                }
            }
        }

        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, dstW, dstW,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, dst.data());
        src.swap(dst);
        srcW = dstW;
        srcTile = dstTile;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);
}

} // namespace

BasicTexture::BasicTexture(const std::string& file, int atlasTileSize)
: Width(0), Height(0), ID(0)
{
    glGenTextures(1, &this->ID);
    LoadFromFile(file.c_str(), atlasTileSize);
}

BasicTexture::~BasicTexture()
{
    glDeleteTextures(1, &this->ID);
}

void BasicTexture::Generate(unsigned int width, unsigned int height, unsigned char *data,
                            int atlasTileSize)
{
    this->Width = width;
    this->Height = height;
    glBindTexture(GL_TEXTURE_2D, this->ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    const bool atlas = atlasTileSize > 0 && width == height &&
                       atlasTileSize <= static_cast<int>(width) &&
                       (static_cast<int>(width) % atlasTileSize) == 0;
    if (atlas)
        uploadIsolatedAtlasMips(static_cast<int>(width), atlasTileSize, data);
    else
        glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    if (data) stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BasicTexture::Bind() const
{
    glBindTexture(GL_TEXTURE_2D, this->ID);
}


void BasicTexture::LoadFromFile(const char* file, int atlasTileSize)
{
    stbi_set_flip_vertically_on_load(false);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(file, &width, &height, &nrChannels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "[BasicTexture] Failed to load: " << file
                  << " -- " << stbi_failure_reason() << std::endl;
        return;
    }
    std::cout << "[BasicTexture] Loaded: " << file
              << "  " << width << "x" << height
              << "  channels=" << nrChannels << std::endl;
    Generate(width, height, data, atlasTileSize);
}
