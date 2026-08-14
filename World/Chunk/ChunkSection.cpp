//
// ChunkSection.cpp – mesh build with corner AO; sun/sky lighting is in the shader
//

#include "ChunkSection.h"
#include "../Block/BlockDataBase.h"
#include "../Block/Water.h"
#include "../World.h"

#include <array>

ChunkSection::ChunkSection(const glm::ivec3& position)
    : m_location(position)
{
    m_blocks.fill(ChunkBlock(BlockId::Air));
    m_skyLight.fill(static_cast<uint8_t>(LIGHT_LEVEL_MAX));
    m_blockLight.fill(0);
}

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

uint8_t ChunkSection::getSkyLight(int x, int y, int z) const {
    if (outOfBounds(x, y, z))
        return 0;
    return m_skyLight[getIndex(x, y, z)];
}

uint8_t ChunkSection::getBlockLight(int x, int y, int z) const {
    if (outOfBounds(x, y, z))
        return 0;
    return m_blockLight[getIndex(x, y, z)];
}

void ChunkSection::setSkyLight(int x, int y, int z, uint8_t value) {
    if (outOfBounds(x, y, z))
        return;
    m_skyLight[getIndex(x, y, z)] = value;
}

void ChunkSection::setBlockLight(int x, int y, int z, uint8_t value) {
    if (outOfBounds(x, y, z))
        return;
    m_blockLight[getIndex(x, y, z)] = value;
}

const ChunkMeshCollection& ChunkSection::getMeshes() const {
    return m_meshes;
}

const glm::ivec3& ChunkSection::getLocation() const {
    return m_location;
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

float ChunkSection::shadeAO(int ao) {
    // ao ∈ [0,3] → milder corner darkening so small holes stay readable
    static constexpr float kTable[4] = {0.78f, 0.86f, 0.94f, 1.0f};
    return kTable[ao];
}

void ChunkSection::sampleCornerLight(const World& world,
                                     int x0, int y0, int z0,
                                     int x1, int y1, int z1,
                                     int x2, int y2, int z2,
                                     int x3, int y3, int z3,
                                     GLfloat& sky, GLfloat& block) const {
    const int ox = m_location.x * CHUNK_SIZE;
    const int oy = m_location.y * CHUNK_SIZE;
    const int oz = m_location.z * CHUNK_SIZE;
    const int xs[4] = {x0, x1, x2, x3};
    const int ys[4] = {y0, y1, y2, y3};
    const int zs[4] = {z0, z1, z2, z3};

    auto readLight = [&](int x, int y, int z, uint8_t& s, uint8_t& b) {
        if (static_cast<unsigned>(x) < static_cast<unsigned>(CHUNK_SIZE) &&
            static_cast<unsigned>(y) < static_cast<unsigned>(CHUNK_SIZE) &&
            static_cast<unsigned>(z) < static_cast<unsigned>(CHUNK_SIZE)) {
            const int idx = getIndex(x, y, z);
            s = m_skyLight[idx];
            b = m_blockLight[idx];
        } else {
            world.getLightsLocked(ox + x, oy + y, oz + z, s, b);
        }
    };

    uint8_t fbSky = 0, fbBlock = 0;
    readLight(x0, y0, z0, fbSky, fbBlock);

    float sk = 0.0f;
    float bl = 0.0f;
    for (int i = 0; i < 4; ++i) {
        const ChunkBlock nb = getAdjacentBlock(world, xs[i], ys[i], zs[i]);
        // Solid interiors are 0; averaging them in makes 1×1 pits almost black.
        if (nb != BlockId::Air && nb.GetData().isOpaque) {
            sk += static_cast<float>(fbSky);
            bl += static_cast<float>(fbBlock);
            continue;
        }
        uint8_t s = 0, b = 0;
        readLight(xs[i], ys[i], zs[i], s, b);
        sk += static_cast<float>(s);
        bl += static_cast<float>(b);
    }
    sky = sk * 0.25f;
    block = bl * 0.25f;
}

void ChunkSection::sampleCellLight(const World& world, int x, int y, int z,
                                   GLfloat& sky, GLfloat& block) const {
    if (static_cast<unsigned>(x) < static_cast<unsigned>(CHUNK_SIZE) &&
        static_cast<unsigned>(y) < static_cast<unsigned>(CHUNK_SIZE) &&
        static_cast<unsigned>(z) < static_cast<unsigned>(CHUNK_SIZE)) {
        const int idx = getIndex(x, y, z);
        sky = static_cast<float>(m_skyLight[idx]);
        block = static_cast<float>(m_blockLight[idx]);
        return;
    }
    uint8_t s = 0, b = 0;
    world.getLightsLocked(m_location.x * CHUNK_SIZE + x,
                          m_location.y * CHUNK_SIZE + y,
                          m_location.z * CHUNK_SIZE + z, s, b);
    sky = static_cast<float>(s);
    block = static_cast<float>(b);
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
                    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
                        blockData.texSideCoords);
                    glm::ivec3 blockPos(x, y, z);
                    GLfloat sky = 0.0f, blockL = 0.0f;
                    sampleCellLight(world, x, y, z, sky, blockL);

                    std::array<GLfloat, 12> face1 = {
                        0.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 1.0f,
                        1.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 0.0f,
                    };
                    m_meshes.floraMesh.addFace(face1, texCoords, m_location, blockPos,
                                               1.0f, sky, blockL, FaceId::FloraA);

                    std::array<GLfloat, 12> face2 = {
                        0.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 1.0f,
                    };
                    m_meshes.floraMesh.addFace(face2, texCoords, m_location, blockPos,
                                               1.0f, sky, blockL, FaceId::FloraB);
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

    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texTopCoords);
    const glm::ivec3 blockPos(x, y, z);
    ChunkMesh& mesh = m_meshes.waterMesh;
    GLfloat sky = 0.0f, blockL = 0.0f;

    // Top
    if (!Water::isWater(above)) {
        std::array<GLfloat, 12> face = {
            0.0f, h, 1.0f,
            1.0f, h, 1.0f,
            1.0f, h, 0.0f,
            0.0f, h, 0.0f,
        };
        sampleCellLight(world, x, y + 1, z, sky, blockL);
        mesh.addFace(face, texCoords, m_location, blockPos, 1.0f, sky, blockL, FaceId::PosY);
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
        sampleCellLight(world, x, y - 1, z, sky, blockL);
        mesh.addFace(face, texCoords, m_location, blockPos, 1.0f, sky, blockL, FaceId::NegY);
    }

    auto addSide = [&](std::array<GLfloat, 12> face, float faceId, int nx, int ny, int nz) {
        if (!shouldDrawWaterSide(world, nx, ny, nz, h))
            return;
        sampleCellLight(world, nx, ny, nz, sky, blockL);
        mesh.addFace(face, texCoords, m_location, blockPos, 1.0f, sky, blockL, faceId);
    };

    addSide({
                1.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 0.0f,
                1.0f, h,    0.0f,
                1.0f, h,    1.0f,
            }, FaceId::PosX, x + 1, y, z);

    addSide({
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, h,    1.0f,
                0.0f, h,    0.0f,
            }, FaceId::NegX, x - 1, y, z);

    addSide({
                0.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 1.0f,
                1.0f, h,    1.0f,
                0.0f, h,    1.0f,
            }, FaceId::PosZ, x, y, z + 1);

    addSide({
                1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, h,    0.0f,
                1.0f, h,    0.0f,
            }, FaceId::NegZ, x, y, z - 1);
}

// ----------------------------------------------------------------------------
// Face Generation Helpers (corner AO; face id for shader sun/sky)
// ----------------------------------------------------------------------------

void ChunkSection::addXPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texSideCoords);
    // Order: (y=0,z=1), (y=0,z=0), (y=1,z=0), (y=1,z=1)
    std::array<GLfloat, 12> face = {
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
    };

    const int corners[4][2] = {{0, 1}, {0, 0}, {1, 0}, {1, 1}}; // ly, lz
    std::array<GLfloat, 4> shades{};
    std::array<GLfloat, 4> skies{};
    std::array<GLfloat, 4> blocks{};
    for (int i = 0; i < 4; ++i) {
        const int yDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + 1, y + yDir, z);
        const bool s2 = occludesAO(world, x + 1, y, z + zDir);
        const bool c  = occludesAO(world, x + 1, y + yDir, z + zDir);
        shades[i] = shadeAO(vertexAO(s1, s2, c));
        sampleCornerLight(world,
                          x + 1, y, z,
                          x + 1, y + yDir, z,
                          x + 1, y, z + zDir,
                          x + 1, y + yDir, z + zDir,
                          skies[i], blocks[i]);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, glm::ivec3(x, y, z),
                        shades, skies, blocks, FaceId::PosX);
}

void ChunkSection::addXNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texSideCoords);
    // Order: (y=0,z=0), (y=0,z=1), (y=1,z=1), (y=1,z=0)
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
    };

    const int corners[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    std::array<GLfloat, 4> shades{};
    std::array<GLfloat, 4> skies{};
    std::array<GLfloat, 4> blocks{};
    for (int i = 0; i < 4; ++i) {
        const int yDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x - 1, y + yDir, z);
        const bool s2 = occludesAO(world, x - 1, y, z + zDir);
        const bool c  = occludesAO(world, x - 1, y + yDir, z + zDir);
        shades[i] = shadeAO(vertexAO(s1, s2, c));
        sampleCornerLight(world,
                          x - 1, y, z,
                          x - 1, y + yDir, z,
                          x - 1, y, z + zDir,
                          x - 1, y + yDir, z + zDir,
                          skies[i], blocks[i]);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, glm::ivec3(x, y, z),
                        shades, skies, blocks, FaceId::NegX);
}

void ChunkSection::addYPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texTopCoords);
    // Order: (-X+Z), (+X+Z), (+X-Z), (-X-Z)
    std::array<GLfloat, 12> face = {
        0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };

    const int corners[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}}; // lx, lz
    std::array<GLfloat, 4> shades{};
    std::array<GLfloat, 4> skies{};
    std::array<GLfloat, 4> blocks{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y + 1, z);
        const bool s2 = occludesAO(world, x, y + 1, z + zDir);
        const bool c  = occludesAO(world, x + xDir, y + 1, z + zDir);
        shades[i] = shadeAO(vertexAO(s1, s2, c));
        sampleCornerLight(world,
                          x, y + 1, z,
                          x + xDir, y + 1, z,
                          x, y + 1, z + zDir,
                          x + xDir, y + 1, z + zDir,
                          skies[i], blocks[i]);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, glm::ivec3(x, y, z),
                        shades, skies, blocks, FaceId::PosY);
}

void ChunkSection::addYNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texBottomCoords);
    // Order: (-X-Z), (+X-Z), (+X+Z), (-X+Z)
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };

    const int corners[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    std::array<GLfloat, 4> shades{};
    std::array<GLfloat, 4> skies{};
    std::array<GLfloat, 4> blocks{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int zDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y - 1, z);
        const bool s2 = occludesAO(world, x, y - 1, z + zDir);
        const bool c  = occludesAO(world, x + xDir, y - 1, z + zDir);
        shades[i] = shadeAO(vertexAO(s1, s2, c));
        sampleCornerLight(world,
                          x, y - 1, z,
                          x + xDir, y - 1, z,
                          x, y - 1, z + zDir,
                          x + xDir, y - 1, z + zDir,
                          skies[i], blocks[i]);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, glm::ivec3(x, y, z),
                        shades, skies, blocks, FaceId::NegY);
}

void ChunkSection::addZPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texSideCoords);
    // Order: (x=0,y=0), (x=1,y=0), (x=1,y=1), (x=0,y=1)
    std::array<GLfloat, 12> face = {
        0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
    };

    const int corners[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}}; // lx, ly
    std::array<GLfloat, 4> shades{};
    std::array<GLfloat, 4> skies{};
    std::array<GLfloat, 4> blocks{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int yDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y, z + 1);
        const bool s2 = occludesAO(world, x, y + yDir, z + 1);
        const bool c  = occludesAO(world, x + xDir, y + yDir, z + 1);
        shades[i] = shadeAO(vertexAO(s1, s2, c));
        sampleCornerLight(world,
                          x, y, z + 1,
                          x + xDir, y, z + 1,
                          x, y + yDir, z + 1,
                          x + xDir, y + yDir, z + 1,
                          skies[i], blocks[i]);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, glm::ivec3(x, y, z),
                        shades, skies, blocks, FaceId::PosZ);
}

void ChunkSection::addZNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block) {
    auto texCoords = BlockDatabase::Get().atlas.GetTexture(
        block.GetData().texSideCoords);
    // Order: (x=1,y=0), (x=0,y=0), (x=0,y=1), (x=1,y=1)
    std::array<GLfloat, 12> face = {
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };

    const int corners[4][2] = {{1, 0}, {0, 0}, {0, 1}, {1, 1}};
    std::array<GLfloat, 4> shades{};
    std::array<GLfloat, 4> skies{};
    std::array<GLfloat, 4> blocks{};
    for (int i = 0; i < 4; ++i) {
        const int xDir = corners[i][0] == 0 ? -1 : 1;
        const int yDir = corners[i][1] == 0 ? -1 : 1;
        const bool s1 = occludesAO(world, x + xDir, y, z - 1);
        const bool s2 = occludesAO(world, x, y + yDir, z - 1);
        const bool c  = occludesAO(world, x + xDir, y + yDir, z - 1);
        shades[i] = shadeAO(vertexAO(s1, s2, c));
        sampleCornerLight(world,
                          x, y, z - 1,
                          x + xDir, y, z - 1,
                          x, y + yDir, z - 1,
                          x + xDir, y + yDir, z - 1,
                          skies[i], blocks[i]);
    }

    ChunkMesh* targetMesh = block.GetData().shaderType == BlockShaderType::Liquid
                                ? &m_meshes.waterMesh : &m_meshes.solidMesh;
    targetMesh->addFace(face, texCoords, m_location, glm::ivec3(x, y, z),
                        shades, skies, blocks, FaceId::NegZ);
}
