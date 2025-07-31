//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_CUBETEXTURE_H
#define MINECRAFT_CUBETEXTURE_H

#include "../Util/NonCopyable.h"

#include <glad/glad.h>

#include <array>
#include <string>

class CubeTexture : public NonCopyable
{
public:
    CubeTexture() = default;
    CubeTexture(const std::array<std::string, 6> &files);

    ~CubeTexture();

    // ORDER: right left top bottom back front
    void BindTexture() const;

    unsigned int m_TexId;

private:
    void loadFromFiles(const std::array<std::string, 6> &files);

};


#endif //MINECRAFT_CUBETEXTURE_H
