//
// Created by 郭珂桢 on 2024/5/26.
//

#include "ResourceManager.h"

#include "stb_image/stb_image.h"

BasicTexture ResourceManager::BlockTexture;
BasicShader ResourceManager::BlockShader;

BasicTexture ResourceManager::LoadTextureFromFile(const GLchar *file)
{
    BasicTexture texture;
    int width, height, nrChannels;
    unsigned char *data = stbi_load(file, &width, &height, &nrChannels, 0);
    texture.Generate(width, height, data);
    return texture;
}
