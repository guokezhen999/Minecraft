//
// ChunkSection – one 16³ vertical slice with its own mesh
//

#ifndef MINECRAFT_CHUNKSECTION_H
#define MINECRAFT_CHUNKSECTION_H

#include "IChunk.h"
#include "ChunkMesh.h"
#include "../WorldConstants.h"

#include <array>
#include <cstdint>
#include <glm/glm.hpp>

class World;

class ChunkSection : public IChunk {
public:
    ChunkSection(const glm::ivec3& position);
    ~ChunkSection() override;

    ChunkBlock getBlock(int x, int y, int z) const override;
    void setBlock(int x, int y, int z, ChunkBlock block) override;

    uint8_t getSkyLight(int x, int y, int z) const;
    uint8_t getBlockLight(int x, int y, int z) const;
    void setSkyLight(int x, int y, int z, uint8_t value);
    void setBlockLight(int x, int y, int z, uint8_t value);

    ChunkBlock getBlockRaw(int x, int y, int z) const { return m_blocks[getIndex(x, y, z)]; }
    uint8_t skyLightRaw(int x, int y, int z) const { return m_skyLight[getIndex(x, y, z)]; }
    uint8_t blockLightRaw(int x, int y, int z) const { return m_blockLight[getIndex(x, y, z)]; }
    void setSkyLightRaw(int x, int y, int z, uint8_t v) { m_skyLight[getIndex(x, y, z)] = v; }
    void setBlockLightRaw(int x, int y, int z, uint8_t v) { m_blockLight[getIndex(x, y, z)] = v; }

    // Build vertex data on CPU (safe on worker thread; no GL calls)
    void buildMesh(const World& world);

    // Upload pending CPU mesh to GPU (GL thread only)
    void bufferMeshes();

    bool hasMesh() const { return m_hasMesh; }
    bool hasPendingUpload() const { return m_pendingUpload; }
    bool isDirty() const { return m_dirty; }
    void markDirty() { m_dirty = true; }
    void clearDirty() { m_dirty = false; }

    // True if section contains only air (skip meshing / drawing)
    bool isEmpty() const { return m_nonAirCount == 0; }

    const ChunkMeshCollection& getMeshes() const;
    const glm::ivec3& getLocation() const;

private:
    bool outOfBounds(int x, int y, int z) const {
        return static_cast<unsigned>(x) >= static_cast<unsigned>(CHUNK_SIZE) ||
               static_cast<unsigned>(y) >= static_cast<unsigned>(CHUNK_SIZE) ||
               static_cast<unsigned>(z) >= static_cast<unsigned>(CHUNK_SIZE);
    }
    int getIndex(int x, int y, int z) const {
        return y * CHUNK_AREA + z * CHUNK_SIZE + x;
    }

    ChunkBlock getAdjacentBlock(const World& world, int x, int y, int z) const;
    static bool shouldDrawFaceAgainst(const ChunkBlock& neighbor);

    void addXPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block);
    void addXNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block);
    void addYPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block);
    void addYNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block);
    void addZPositiveFace(const World& world, int x, int y, int z, const ChunkBlock& block);
    void addZNegativeFace(const World& world, int x, int y, int z, const ChunkBlock& block);

    void addWaterBlock(const World& world, int x, int y, int z, const ChunkBlock& block);
    bool shouldDrawWaterSide(const World& world, int x, int y, int z, float myHeight) const;

    bool occludesAO(const World& world, int x, int y, int z) const;
    static int vertexAO(bool side1, bool side2, bool corner);
    static float shadeAO(int ao);
    void sampleCornerLight(const World& world,
                           int x0, int y0, int z0,
                           int x1, int y1, int z1,
                           int x2, int y2, int z2,
                           int x3, int y3, int z3,
                           GLfloat& sky, GLfloat& block) const;
    void sampleCellLight(const World& world, int x, int y, int z,
                         GLfloat& sky, GLfloat& block) const;

private:
    std::array<ChunkBlock, CHUNK_VOLUME> m_blocks{};
    std::array<uint8_t, CHUNK_VOLUME> m_skyLight{};
    std::array<uint8_t, CHUNK_VOLUME> m_blockLight{};
    ChunkMeshCollection m_meshes;
    glm::ivec3 m_location;
    bool m_hasMesh = false;
    bool m_pendingUpload = false;
    bool m_dirty = true;
    int m_nonAirCount = 0;
};

#endif //MINECRAFT_CHUNKSECTION_H
