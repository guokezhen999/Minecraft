//
// Created by 郭珂桢 on 2024/5/22.
//

#include "BlockType.h"

BlockType::BlockType(const std::string &filename)
: m_data(filename)
{
}

const BlockData &BlockType::GetData() const
{
    return m_data;
}
