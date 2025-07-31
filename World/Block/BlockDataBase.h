//
// Created by 郭珂桢 on 2024/5/28.
//

#ifndef MINECRAFT_BLOCKDATABASE_H
#define MINECRAFT_BLOCKDATABASE_H

#include "../../Util/Singleton.h"
#include "BlockTypes/BlockType.h"
#include "../../Textures/TextureAtlas.h"

#include <array>

class BlockDatabase : public Singleton {
public:
    static BlockDatabase &Get();

    const BlockType &GetBlock(BlockId id) const;
    const BlockData &GetData(BlockId id) const;

    TextureAtlas TextureAtlas;

private:
    BlockDatabase();

    std::array<std::unique_ptr<BlockType>, (unsigned)BlockId::NUM_TYPES> m_blocks;
};


#endif //MINECRAFT_BLOCKDATABASE_H
