//
// Created by 郭珂桢 on 2024/5/22.
//

#include "BlockData.h"

#include <algorithm>
#include <fstream>

BlockData::BlockData(const std::string &filename)
{
    std::ifstream inFile("resources/blocks/" + filename + ".block");
    if (!inFile.is_open())
        throw std::runtime_error("Unable to open block file: " + filename + "!");

    bool sawOpacity = false;
    bool sawEmission = false;
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
        else if (line == "LightOpacity")
        {
            int v = 0;
            inFile >> v;
            m_data.lightOpacity = static_cast<uint8_t>(std::max(0, std::min(15, v)));
            sawOpacity = true;
        }
        else if (line == "LightEmission")
        {
            int v = 0;
            inFile >> v;
            m_data.lightEmission = static_cast<uint8_t>(std::max(0, std::min(15, v)));
            sawEmission = true;
        }
    }

    if (!sawOpacity) {
        if (m_data.id == BlockId::Water)
            m_data.lightOpacity = 2;
        else if (m_data.id == BlockId::OakLeaf || m_data.id == BlockId::SavannaLeaf)
            m_data.lightOpacity = 1;
        else if (!m_data.isOpaque || m_data.meshType == BlockMeshType::X)
            m_data.lightOpacity = 0;
        else
            m_data.lightOpacity = 15;
    }
    if (!sawEmission)
        m_data.lightEmission = 0;
}

const BlockDataHolder &BlockData::GetBlockData() const
{
    return m_data;
}
