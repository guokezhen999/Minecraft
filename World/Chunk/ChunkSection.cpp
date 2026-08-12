//
// ChunkSection.cpp
//

#include "ChunkSection.h"
#include "../Block/BlockDataBase.h"
#include "../World.h"

#include <array>

ChunkSection::ChunkSection(const sf::Vector3i& position)
    : m_location(position), m_blocks(CHUNK_VOLUME, ChunkBlock(BlockId::Air)) {}

ChunkSection::~ChunkSection() {
    m_meshes.solidMesh.deleteData();
    m_meshes.waterMesh.deleteData();
    m_meshes.floraMesh.deleteData();
}

ChunkBlock ChunkSection::getBlock(int x, int y, int z) const {
    if (outOfBounds(x, y, z)) {
        return ChunkBlock(BlockId::Air);
    }
    return m_blocks[getIndex(x, y, z)];
}

void ChunkSection::setBlock(int x, int y, int z, ChunkBlock block) {
    if (outOfBounds(x, y, z)) {
        return;
    }
    const int idx = getIndex(x, y, z);
    const ChunkBlock prev = m_blocks[idx];
    if (prev == block)
        return;

    if (prev == BlockId::Air && block != BlockId::Air)
        ++m_nonAirCount;
    else if (prev != BlockId::Air && block == BlockId::Air)
        --m_nonAirCount;

    m_blocks[idx] = block;
}

const ChunkMeshCollection& ChunkSection::getMeshes() const {
    return m_meshes;
}

const sf::Vector3i& ChunkSection::getLocation() const {
    return m_location;
}

bool ChunkSection::outOfBounds(int x, int y, int z) const {
    return x < 0 || x >= CHUNK_SIZE ||
           y < 0 || y >= CHUNK_SIZE ||
           z < 0 || z >= CHUNK_SIZE;
}

int ChunkSection::getIndex(int x, int y, int z) const {
    return y * CHUNK_AREA + z * CHUNK_SIZE + x;
}

ChunkBlock ChunkSection::getAdjacentBlock(const World& world, int x, int y, int z) const {
    if (!outOfBounds(x, y, z)) {
        return getBlock(x, y, z);
    }
    int wx = m_location.x * CHUNK_SIZE + x;
    int wy = m_location.y * CHUNK_SIZE + y;
    int wz = m_location.z * CHUNK_SIZE + z;
    // buildMesh runs under World's shared chunk lock
    return world.getBlockLocked(wx, wy, wz);
}

bool ChunkSection::shouldDrawFaceAgainst(const ChunkBlock& neighbor) {
    return neighbor == BlockId::Air || !neighbor.GetData().isOpaque;
}

void ChunkSection::buildMesh(const World& world) {
    m_meshes.solidMesh.clearCPU();
    m_meshes.waterMesh.clearCPU();
    m_meshes.floraMesh.clearCPU();
    m_dirty = false;

    if (isEmpty()) {
        // Only schedule upload if we previously had GPU geometry to clear
        if (m_hasMesh)
            m_pendingUpload = true;
        return;
    }

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

void ChunkSection::bufferMeshes() {
    m_meshes.solidMesh.bufferMesh();
    m_meshes.waterMesh.bufferMesh();
    m_meshes.floraMesh.bufferMesh();
    m_pendingUpload = false;
    m_hasMesh = true;
}

// ----------------------------------------------------------------------------
// Face Generation Helpers
// ----------------------------------------------------------------------------

void ChunkSection::addXPositiveFace(int x, int y, int z, const ChunkBlock& block) {
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

void ChunkSection::addXNegativeFace(int x, int y, int z, const ChunkBlock& block) {
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

void ChunkSection::addYPositiveFace(int x, int y, int z, const ChunkBlock& block) {
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

void ChunkSection::addYNegativeFace(int x, int y, int z, const ChunkBlock& block) {
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

void ChunkSection::addZPositiveFace(int x, int y, int z, const ChunkBlock& block) {
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

void ChunkSection::addZNegativeFace(int x, int y, int z, const ChunkBlock& block) {
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
