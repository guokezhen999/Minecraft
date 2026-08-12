//
// ChunkSection – one 16³ vertical slice with its own mesh
//

#ifndef MINECRAFT_CHUNKSECTION_H
#define MINECRAFT_CHUNKSECTION_H

#include "IChunk.h"
#include "ChunkMesh.h"
#include "../WorldConstants.h"

#include <vector>
#include <SFML/Graphics.hpp>

class World;

class ChunkSection : public IChunk {
public:
    ChunkSection(const sf::Vector3i& position);
    ~ChunkSection() override;

    ChunkBlock getBlock(int x, int y, int z) const override;
    void setBlock(int x, int y, int z, ChunkBlock block) override;

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
    const sf::Vector3i& getLocation() const;

private:
    bool outOfBounds(int x, int y, int z) const;
    int getIndex(int x, int y, int z) const;

    ChunkBlock getAdjacentBlock(const World& world, int x, int y, int z) const;
    static bool shouldDrawFaceAgainst(const ChunkBlock& neighbor);

    void addXPositiveFace(int x, int y, int z, const ChunkBlock& block);
    void addXNegativeFace(int x, int y, int z, const ChunkBlock& block);
    void addYPositiveFace(int x, int y, int z, const ChunkBlock& block);
    void addYNegativeFace(int x, int y, int z, const ChunkBlock& block);
    void addZPositiveFace(int x, int y, int z, const ChunkBlock& block);
    void addZNegativeFace(int x, int y, int z, const ChunkBlock& block);

private:
    std::vector<ChunkBlock> m_blocks;
    ChunkMeshCollection m_meshes;
    sf::Vector3i m_location;
    bool m_hasMesh = false;
    bool m_pendingUpload = false;
    bool m_dirty = true;
    int m_nonAirCount = 0;
};

#endif //MINECRAFT_CHUNKSECTION_H
