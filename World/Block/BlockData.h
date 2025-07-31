//
// Created by 郭珂桢 on 2024/5/22.
//

#ifndef MINECRAFT_BLOCKDATA_H
#define MINECRAFT_BLOCKDATA_H

#include "BlockId.h"
#include "../Util/NonCopyable.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>

enum class BlockMeshType
{
    Cube,
    X
};

enum class BlockShaderType
{
    Chunk,
    Liquid,
    Flora
};

struct BlockDataHolder : public NonCopyable
{
    BlockId id;
    glm::ivec2 texTopCoords;
    glm::ivec2 texSideCoords;
    glm::ivec2 texBottomCoords;

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
