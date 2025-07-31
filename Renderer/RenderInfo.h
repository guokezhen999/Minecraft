//
// Created by 郭珂桢 on 2024/5/23.
//

#ifndef MINECRAFT_RENDERINFO_H
#define MINECRAFT_RENDERINFO_H

struct RenderInfo
{
    unsigned int VAO = 0;
    unsigned int IndicesCount = 0;

    inline void Reset()
    {
        VAO = 0;
        IndicesCount = 0;
    }
};

#endif //MINECRAFT_RENDERINFO_H
