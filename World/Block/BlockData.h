//
// Created by 郭珂桢 on 2024/5/22.
//

#ifndef MINECRAFT_BLOCKDATA_H
#define MINECRAFT_BLOCKDATA_H

#include "BlockId.h"
#include "../Util/NonCopyable.h"

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <string>

enum class BlockMeshType
{
    Cube = 0,
    X = 1
};

enum class BlockShaderType
{
    Chunk = 0,
    Liquid = 1,
    Flora = 2
};

struct BlockDataHolder : public NonCopyable
{
    BlockId id;
    sf::Vector2i texTopCoords;
    sf::Vector2i texSideCoords;
    sf::Vector2i texBottomCoords;

    BlockMeshType meshType;
    BlockShaderType shaderType;

    bool isOpaque;  // 是否看的到背景色
    bool isCollidable;  // 是否可碰撞
};

class BlockData : public NonCopyable
{
public:
    BlockData(const std::string &filename);

    const BlockDataHolder &GetBlockData() const;

private:
    BlockDataHolder m_data;
};


#endif //MINECRAFT_BLOCKDATA_H
