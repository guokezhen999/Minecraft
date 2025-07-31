//
// Created by 郭珂桢 on 2024/5/20.
//

#include "BasicTexture.h"

#include <iostream>

#include <glad/glad.h>

#include "stb_image/stb_image.h"

BasicTexture::BasicTexture(const std::string& file)
: Width(0), Height(0), ID(0)
{
    glGenTextures(1, &this->ID);
}

BasicTexture::~BasicTexture()
{
    glDeleteTextures(1, &this->ID);
}

void BasicTexture::Generate(unsigned int width, unsigned int height, unsigned char *data)
{
    this->Width = width;
    this->Height = height;
    glBindTexture(GL_TEXTURE_2D, this->ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(data);
    // Unbind Textures
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BasicTexture::Bind() const
{
    glBindTexture(GL_TEXTURE_2D, this->ID);
}


void BasicTexture::LoadFromFile(const char* file)
{
    int width, height, nrChannels;
    unsigned char *data = stbi_load(file, &width, &height, &nrChannels, 0);
    Generate(width, height, data);
}

