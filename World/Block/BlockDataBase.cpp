//
// Created by 郭珂桢 on 2024/5/28.
//

#include "BlockDataBase.h"

BlockDatabase::BlockDatabase()
{
    m_blocks[(int)BlockId::Air] = std::make_unique<DefaultBlock>("air");
    m_blocks[(int)BlockId::Grass] = std::make_unique<DefaultBlock>("grass");
    m_blocks[(int)BlockId::Dirt] = std::make_unique<DefaultBlock>("dirt");
    m_blocks[(int)BlockId::Stone] = std::make_unique<DefaultBlock>("stone");
    m_blocks[(int)BlockId::OakBark] = std::make_unique<DefaultBlock>("oakBark");
    m_blocks[(int)BlockId::OakLeaf] = std::make_unique<DefaultBlock>("oakLeaf");
    m_blocks[(int)BlockId::Sand] = std::make_unique<DefaultBlock>("sand");
    m_blocks[(int)BlockId::Water] = std::make_unique<DefaultBlock>("water");
    m_blocks[(int)BlockId::Cactus] = std::make_unique<DefaultBlock>("cactus");
    m_blocks[(int)BlockId::TallGrass] =
            std::make_unique<DefaultBlock>("tallGrass");
    m_blocks[(int)BlockId::Rose] = std::make_unique<DefaultBlock>("rose");
    m_blocks[(int)BlockId::DeadShrub] =
            std::make_unique<DefaultBlock>("deadShrub");
    m_blocks[(int)BlockId::Torch] = std::make_unique<DefaultBlock>("torch");
    m_blocks[(int)BlockId::Sandstone] = std::make_unique<DefaultBlock>("sandstone");
    m_blocks[(int)BlockId::Ice] = std::make_unique<DefaultBlock>("ice");
    m_blocks[(int)BlockId::Snow] = std::make_unique<DefaultBlock>("snow");
    m_blocks[(int)BlockId::SavannaGrass] = std::make_unique<DefaultBlock>("savannaGrass");
    m_blocks[(int)BlockId::SavannaBark] = std::make_unique<DefaultBlock>("savannaBark");
    m_blocks[(int)BlockId::SavannaLeaf] = std::make_unique<DefaultBlock>("savannaLeaf");

    atlas = TextureAtlas("/defaultPack");
}

BlockDatabase &BlockDatabase::Get()
{
    static BlockDatabase d;
    return d;
}

const BlockType &BlockDatabase::GetBlock(BlockId id) const
{
    return *m_blocks[(int)id];
}

const BlockData &BlockDatabase::GetData(BlockId id) const
{
    return m_blocks[(int)id]->GetData();
}
