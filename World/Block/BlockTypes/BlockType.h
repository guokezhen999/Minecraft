//
// Created by 郭珂桢 on 2024/5/22.
//

#ifndef MINECRAFT_BLOCKTYPE_H
#define MINECRAFT_BLOCKTYPE_H

#include "../BlockData.h"

class BlockType : public NonCopyable
{
public:
    BlockType(const std::string &filename);
    virtual ~BlockType() = default;

    const BlockData &GetData() const;


private:
    BlockData m_data;
};

class DefaultBlock : public BlockType
{
public:
    DefaultBlock(const std::string &fileName)
    : BlockType(fileName)
    {
    }
};

#endif //MINECRAFT_BLOCKTYPE_H
