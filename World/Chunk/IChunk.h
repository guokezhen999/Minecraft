//
// Created by 郭珂桢 on 25-8-1.
//

#ifndef MINECRAFT_ICHUNK_H
#define MINECRAFT_ICHUNK_H

#include "../Block/ChunkBlock.h"

struct IChunk {
    virtual ~IChunk() = default;

    virtual ChunkBlock getBlock(int x, int y, int z) const = 0;
    virtual void setBlock(int x, int y, int z, ChunkBlock block) = 0;
};

#endif //MINECRAFT_ICHUNK_H
