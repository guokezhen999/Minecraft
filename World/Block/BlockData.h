//
// Created by 郭珂桢 on 2024/5/22.
//

#ifndef MINECRAFT_BLOCKDATA_H
#define MINECRAFT_BLOCKDATA_H

#include "BlockId.h"
#include "../Util/NonCopyable.h"

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
    BlockId id = BlockId::Air;
    glm::ivec2 texTopCoords{0, 0};
    glm::ivec2 texSideCoords{0, 0};
    glm::ivec2 texBottomCoords{0, 0};

    BlockMeshType meshType = BlockMeshType::Cube;
    BlockShaderType shaderType = BlockShaderType::Chunk;

    bool isOpaque = true;  // 是否看的到背景色
    bool isCollidable = true;  // 是否可碰撞

    // Separate from isOpaque: leaves block sky but stay see-through
    uint8_t lightOpacity = 15;
    uint8_t lightEmission = 0;
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
