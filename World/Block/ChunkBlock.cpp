//
// Created by 郭珂桢 on 2024/5/25.
//

#include "ChunkBlock.h"

#include "BlockDataBase.h"

ChunkBlock::ChunkBlock(Block_t id)
: id(id)
{

}

ChunkBlock::ChunkBlock(BlockId id)
: id(static_cast<Block_t>(id))
{

}

const BlockDataHolder &ChunkBlock::GetData() const
{
    return BlockDatabase::Get().GetData((BlockId)id).GetBlockData();
}

const BlockType &ChunkBlock::GetType() const
{
    return BlockDatabase::Get().GetBlock((BlockId)id);
}
