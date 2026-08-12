//
// Created by 郭珂桢 on 2024/5/25.
//

#ifndef MINECRAFT_CHUNKBLOCK_H
#define MINECRAFT_CHUNKBLOCK_H

#include "BlockId.h"

#include <cstdint>

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
        return id == other.id && meta == other.meta;
    }

    bool operator!=(ChunkBlock other) const
    {
        return !(*this == other);
    }

    // Compare type only (ignores fluid meta)
    bool is(BlockId blockId) const
    {
        return id == static_cast<Block_t>(blockId);
    }

    Block_t id = 0;
    uint8_t meta = 0; // water: flow level 0–7 (0 = source)
};


#endif //MINECRAFT_CHUNKBLOCK_H
