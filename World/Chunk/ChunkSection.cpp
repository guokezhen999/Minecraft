//
// ChunkSection.cpp – mesh build with cardinal light + corner AO
//

#include "ChunkSection.h"
#include "../Block/BlockDataBase.h"
#include "../Block/Water.h"
#include "../World.h"

#include <array>

namespace {
constexpr float LIGHT_TOP = 1.00f;
constexpr float LIGHT_X   = 0.80f;
constexpr float LIGHT_Z   = 0.65f;
constexpr float LIGHT_BOT = 0.50f;
} // namespace

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

bool ChunkSection::occludesAO(const World& world, int x, int y, int z) const {
    const ChunkBlock b = getAdjacentBlock(world, x, y, z);
    return b != BlockId::Air && b.GetData().isOpaque;
}

int ChunkSection::vertexAO(bool side1, bool side2, bool corner) {
    if (side1 && side2)
        return 0;
    return 3 - static_cast<int>(side1) - static_cast<int>(side2) - static_cast<int>(corner);
}

float ChunkSection::shadeAO(int ao, float cardinal) {
    // ao ∈ [0,3] → darker in corners / between adjacent solids
    static constexpr float kTable[4] = {0.55f, 0.72f, 0.86f, 1.0f};
    return kTable[ao] * cardinal;
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

                if (Water::isWater(block)) {
                    addWaterBlock(world, x, y, z, block);
                    continue;
                }

                if (meshType == BlockMeshType::Cube) {
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x + 1, y, z))) {
                        addXPositiveFace(world, x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x - 1, y, z))) {
                        addXNegativeFace(world, x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y + 1, z))) {
                        addYPositiveFace(world, x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y - 1, z))) {
                        addYNegativeFace(world, x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y, z + 1))) {
                        addZPositiveFace(world, x, y, z, block);
                    }
                    if (shouldDrawFaceAgainst(getAdjacentBlock(world, x, y, z - 1))) {
                        addZNegativeFace(world, x, y, z, block);
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
                    m_meshes.floraMesh.addFace(face1, texCoords, m_location, blockPos, LIGHT_TOP);

                    std::array<GLfloat, 12> face2 = {
                        0.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 1.0f,
                    };
                    m_meshes.floraMesh.addFace(face2, texCoords, m_location, blockPos, LIGHT_TOP);
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
// Water (variable surface height by flow level)
// ----------------------------------------------------------------------------

bool ChunkSection::shouldDrawWaterSide(const World& world, int x, int y, int z,
                                       float myHeight) const {
    const ChunkBlock n = getAdjacentBlock(world, x, y, z);
    if (n == BlockId::Air)
        return true;
    if (Water::isWater(n)) {
        const ChunkBlock aboveN = getAdjacentBlock(world, x, y + 1, z);
        const float nh = Water::surfaceHeight(n, aboveN);
        return nh < myHeight - 0.001f;
    }
    return !n.GetData().isOpaque;
}

void ChunkSection::addWaterBlock(const World& world, int x, int y, int z,
                                 const ChunkBlock& block) {
    const ChunkBlock above = getAdjacentBlock(world, x, y + 1, z);
    const float h = Water::surfaceHeight(block, above);

    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texTopCoords.x, block.GetData().texTopCoords.y));
    const sf::Vector3i blockPos(x, y, z);
    ChunkMesh& mesh = m_meshes.waterMesh;

    // Top
    if (!Water::isWater(above)) {
        std::array<GLfloat, 12> face = {
            0.0f, h, 1.0f,
            1.0f, h, 1.0f,
            1.0f, h, 0.0f,
            0.0f, h, 0.0f,
        };
        mesh.addFace(face, texCoords, m_location, blockPos, LIGHT_TOP);
    }

    // Bottom
    const ChunkBlock below = getAdjacentBlock(world, x, y - 1, z);
    if (!Water::isWater(below) && !below.GetData().isOpaque) {
        std::array<GLfloat, 12> face = {
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
        };
        mesh.addFace(face, texCoords, m_location, blockPos, LIGHT_BOT);
    }

    auto addSide = [&](std::array<GLfloat, 12> face, float light, int nx, int ny, int nz) {
        if (!shouldDrawWaterSide(world, nx, ny, nz, h))
            return;
        mesh.addFace(face, texCoords, m_location, blockPos, light);
    };

    addSide({
                1.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 0.0f,
                1.0f, h,    0.0f,
                1.0f, h,    1.0f,
            }, LIGHT_X, x + 1, y, z);

    addSide({
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, h,    1.0f,
                0.0f, h,    0.0f,
            }, LIGHT_X, x - 1, y, z);

    addSide({
                0.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 1.0f,
                1.0f, h,    1.0f,
                0.0f, h,    1.0f,
            }, LIGHT_Z, x, y, z + 1);

    addSide({
                1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, h,    0.0f,
                1.0f, h,    0.0f,
            }, LIGHT_Z, x, y, z - 1);
}

// ----------------------------------------------------------------------------
// Face Generation Helpers (cardinal × corner AO)
// ----------------------------------------------------------------------------

void ChunkSection::addXPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    // Order: (y=0,z=1), (y=0,z=0), (y=1,z=0), (y=1,z=1)
    std::array<GLfloat, 12> face = {
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
    };

    const int corners[4][2] = {{0, 1}, {0, 0}, {1, 0}, {1, 1}}; // ly, lz
    std::array<GLfloat, 4> lights{};
    for (int i = 0; i < 4; ++i) {
        const int yDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + 1, y + yDir, z);
        const bool s2 = occludesAO(world, x + 1, y, z + zDir);
        const bool c  = occludesAO(world, x + 1, y + yDir, z + zDir);
        lights[i] = shadeAO(vertexAO(s1, s2, c), LIGHT_X);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), lights);
}

void ChunkSection::addXNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    // Order: (y=0,z=0), (y=0,z=1), (y=1,z=1), (y=1,z=0)
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
    };

    const int corners[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    std::array<GLfloat, 4> lights{};
    for (int i = 0; i < 4; ++i) {
        const int yDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x - 1, y + yDir, z);
        const bool s2 = occludesAO(world, x - 1, y, z + zDir);
        const bool c  = occludesAO(world, x - 1, y + yDir, z + zDir);
        lights[i] = shadeAO(vertexAO(s1, s2, c), LIGHT_X);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), lights);
}

void ChunkSection::addYPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texTopCoords.x, block.GetData().texTopCoords.y));
    // Order: (-X+Z), (+X+Z), (+X-Z), (-X-Z)
    std::array<GLfloat, 12> face = {
        0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };

    const int corners[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}}; // lx, lz
    std::array<GLfloat, 4> lights{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y + 1, z);
        const bool s2 = occludesAO(world, x, y + 1, z + zDir);
        const bool c  = occludesAO(world, x + xDir, y + 1, z + zDir);
        lights[i] = shadeAO(vertexAO(s1, s2, c), LIGHT_TOP);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), lights);
}

void ChunkSection::addYNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texBottomCoords.x, block.GetData().texBottomCoords.y));
    // Order: (-X-Z), (+X-Z), (+X+Z), (-X+Z)
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };

    const int corners[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    std::array<GLfloat, 4> lights{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y - 1, z);
        const bool s2 = occludesAO(world, x, y - 1, z + zDir);
        const bool c  = occludesAO(world, x + xDir, y - 1, z + zDir);
        lights[i] = shadeAO(vertexAO(s1, s2, c), LIGHT_BOT);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), lights);
}

void ChunkSection::addZPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    // Order: (x=0,y=0), (x=1,y=0), (x=1,y=1), (x=0,y=1)
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
    };

    const int corners[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}}; // lx, ly
    std::array<GLfloat, 4> lights{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int yDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y, z + 1);
        const bool s2 = occludesAO(world, x, y + yDir, z + 1);
        const bool c  = occludesAO(world, x + xDir, y + yDir, z + 1);
        lights[i] = shadeAO(vertexAO(s1, s2, c), LIGHT_Z);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), lights);
}

void ChunkSection::addZNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().TextureAtlas.GetTexture(
        glm::ivec2(block.GetData().texSideCoords.x, block.GetData().texSideCoords.y));
    // Order: (x=1,y=0), (x=0,y=0), (x=0,y=1), (x=1,y=1)
    std::array<GLfloat, 12> face = {
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };

    const int corners[4][2] = {{1, 0}, {0, 0}, {0, 1}, {1, 1}};
    std::array<GLfloat, 4> lights{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int yDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y, z - 1);
        const bool s2 = occludesAO(world, x, y + yDir, z - 1);
        const bool c  = occludesAO(world, x + xDir, y + yDir, z - 1);
        lights[i] = shadeAO(vertexAO(s1, s2, c), LIGHT_Z);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, sf::Vector3i(x, y, z), lights);
}
