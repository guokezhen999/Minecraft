//
// Created by 郭珂桢 on 2024/5/25.
//

#ifndef MINECRAFT_CHUNKBLOCK_H
#define MINECRAFT_CHUNKBLOCK_H

#include "BlockId.h"

struct BlockDataHolder;
class BlockType;

class ChunkBlock
{
public:
    ChunkBlock() = default;

    ChunkBlock(Block_t id);
    ChunkBlock(BlockId id);

    const BlockType &GetType() const;
    const BlockDataHolder &GetData() const;

    bool operator==(ChunkBlock other) const
    {
        return id == other.id;
    }

    bool operator!=(ChunkBlock other) const
    {
        return id != other.id;
    }


    Block_t id = 0;
};


#endif //MINECRAFT_CHUNKBLOCK_H
