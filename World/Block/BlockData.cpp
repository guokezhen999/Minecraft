//
// Created by 郭珂桢 on 2024/5/22.
//

#include "BlockData.h"

#include <fstream>

BlockData::BlockData(const std::string &filename)
{
    std::ifstream inFile("resources/blocks/" + filename + ".block");
    if (!inFile.is_open())
        throw std::runtime_error("Unable to open block file: " + filename + "!");
    std::string line;
    while (std::getline(inFile, line)) {
        if (line == "TexTop") {
            int x, y;
            inFile >> x >> y;
            m_data.texTopCoords = {x, y};
        } else if (line == "TexSide") {
            int x, y;
            inFile >> x >> y;
            m_data.texSideCoords = {x, y};
        } else if (line == "TexBottom") {
            int x, y;
            inFile >> x >> y;
            m_data.texBottomCoords = {x, y};
        } else if (line == "TexAll") {
            int x, y;
            inFile >> x >> y;
            m_data.texTopCoords = {x, y};
            m_data.texBottomCoords = {x, y};
            m_data.texSideCoords = {x, y};
        } else if (line == "Id") {
            int id;
            inFile >> id;
            m_data.id = static_cast<BlockId>(id);
        } else if (line == "Opaque")
            inFile >> m_data.isOpaque;
        else if (line == "Collidable")
            inFile >> m_data.isCollidable;
        else if (line == "MeshType")
        {
            int id;
            inFile >> id;
            m_data.meshType = static_cast<BlockMeshType>(id);
        }
        else if (line == "ShaderType")
        {
            int id;
            inFile >> id;
            m_data.shaderType = static_cast<BlockShaderType>(id);
        }
    }
}

const BlockDataHolder &BlockData::GetBlockData() const
{
    return m_data;
}
