//
// Created by 郭珂桢 on 25-8-1.
//

#include "Chunk.h"
#include "../Block/BlockDataBase.h"
#include "../World.h"

#include <array>

Chunk::Chunk(const sf::Vector3i& position)
    : m_location(position), m_blocks(CHUNK_VOLUME, ChunkBlock(BlockId::Air)) {}

Chunk::~Chunk() {
    m_meshes.solidMesh.deleteData();
    m_meshes.waterMesh.deleteData();
    m_meshes.floraMesh.deleteData();
}

ChunkBlock Chunk::getBlock(int x, int y, int z) const {
    if (outOfBounds(x, y, z)) {
        return ChunkBlock(BlockId::Air);
    }
    return m_blocks[getIndex(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, ChunkBlock block) {
    if (outOfBounds(x, y, z)) {
        return;
    }
    m_blocks[getIndex(x, y, z)] = block;
}

const ChunkMeshCollection& Chunk::getMeshes() const {
    return m_meshes;
}

const sf::Vector3i& Chunk::getLocation() const {
    return m_location;
}

bool Chunk::outOfBounds(int x, int y, int z) const {
    return x < 0 || x >= CHUNK_SIZE ||
           y < 0 || y >= CHUNK_SIZE ||
           z < 0 || z >= CHUNK_SIZE;
}

int Chunk::getIndex(int x, int y, int z) const {
    return y * CHUNK_AREA + z * CHUNK_SIZE + x;
}

ChunkBlock Chunk::getAdjacentBlock(const World& world, int x, int y, int z) const {
    if (!outOfBounds(x, y, z)) {
        return getBlock(x, y, z);
    }
    int wx = m_location.x * CHUNK_SIZE + x;
    int wy = m_location.y * CHUNK_SIZE + y;
    int wz = m_location.z * CHUNK_SIZE + z;
    // buildMesh runs under World's shared chunk lock
    return world.getBlockLocked(wx, wy, wz);
}

bool Chunk::shouldDrawFaceAgainst(const ChunkBlock& neighbor) {
    return neighbor == BlockId::Air || !neighbor.GetData().isOpaque;
}

void Chunk::buildMesh(const World& world) {
    m_meshes.solidMesh.clearCPU();
    m_meshes.waterMesh.clearCPU();
    m_meshes.floraMesh.clearCPU();

    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                ChunkBlock block = getBlock(x, y, z);

                if (block == BlockId::Air) {
                    continue;
                }

                auto& blockData = block.GetData();
                auto meshType = blockData.meshType;

                if (meshType == BlockMeshType::Cube) {
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x + 1, y, z))) {
                        addXPositiveFace(x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x - 1, y, z))) {
                        addXNegativeFace(x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y + 1, z))) {
                        addYPositiveFace(x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y - 1, z))) {
                        addYNegativeFace(x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y, z + 1))) {
                        addZPositiveFace(x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y, z - 1))) {
                        addZNegativeFace(x, y, z, block);
                    }
                }
                else if (meshType == BlockMeshType::X) {
                    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
                        glm::ivec2(blockData.texSideCoords.x, blockData.texSideCoords.y));
                    sf::Vector3i blockPos(x, y, z);

                    std::array<GLfloat, 12> face1 = {
                        0.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 1.0f,
                        1.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 0.0f,
                    };
                    m_meshes.floraMesh.addFace(face1, texCoords, m_location, blockPos, 1.0f);

                    std::array<GLfloat, 12> face2 = {
                        0.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 1.0f,
                    };
                    m_meshes.floraMesh.addFace(face2, texCoords, m_location, blockPos, 1.0f);
                }
            }
        }
    }

    m_pendingUpload = true;
}

void Chunk::bufferMeshes() {
    m_meshes.solidMesh.bufferMesh();
    m_meshes.waterMesh.bufferMesh();
    m_meshes.floraMesh.bufferMesh();
    m_pendingUpload = false;
    m_hasMesh = true;
}

// ----------------------------------------------------------------------------
// Face Generation Helpers
// Reference coordinates for a unit cube [0, 1]
// The cardinal light provides basic fake ambient occlusion/directional lighting
// ----------------------------------------------------------------------------

void Chunk::addXPositiveFace(int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    std::array<GLfloat, 12> face = {
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
    };
    
    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), 0.8f);
}

void Chunk::addXNegativeFace(int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
    };
    
    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), 0.8f);
}

void Chunk::addYPositiveFace(int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(glm::ivec2(block.GetData().texTopCoords.x, block.GetData().texTopCoords.y));
    std::array<GLfloat, 12> face = {
        0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    
    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), 1.0f);
}

void Chunk::addYNegativeFace(int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(glm::ivec2(block.GetData().texBottomCoords.x, block.GetData().texBottomCoords.y));
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };
    
    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), 0.5f);
}

void Chunk::addZPositiveFace(int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
    };
    
    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), 0.6f);
}

void Chunk::addZNegativeFace(int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    std::array<GLfloat, 12> face = {
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };
    
    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), 0.6f);
}
