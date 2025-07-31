//
// Created by 郭珂桢 on 2024/5/23.
//

#include "CubeTexture.h"

CubeTexture::CubeTexture(const std::array<std::string, 6> &files)
{
    loadFromFiles(files);
}

CubeTexture::~CubeTexture()
{
    glDeleteTextures(1, &m_TexId);
}

void CubeTexture::BindTexture() const
{

}

void CubeTexture::loadFromFiles(const std::array<std::string, 6> &files)
{

}
