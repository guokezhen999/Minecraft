//
// Chunk – vertical column of ChunkSections at (cx, cz)
//

#ifndef MINECRAFT_CHUNK_H
#define MINECRAFT_CHUNK_H

#include "ChunkSection.h"
#include "../WorldConstants.h"

#include <array>
#include <cstdint>
#include <memory>

class World;

class Chunk {
public:
    Chunk(int cx, int cz);
    ~Chunk() = default;

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    int getCX() const { return m_cx; }
    int getCZ() const { return m_cz; }

    // Local XZ + world Y
    ChunkBlock getBlock(int lx, int worldY, int lz) const;
    void setBlock(int lx, int worldY, int lz, ChunkBlock block);

    uint8_t getSkyLight(int lx, int worldY, int lz) const;
    uint8_t getBlockLight(int lx, int worldY, int lz) const;
    void setSkyLight(int lx, int worldY, int lz, uint8_t value);
    void setBlockLight(int lx, int worldY, int lz, uint8_t value);
    void getLights(int lx, int worldY, int lz, uint8_t& sky, uint8_t& block) const;

    // Like setBlock but does not mark sections dirty (bulk terrain fill)
    void setBlockRaw(int lx, int worldY, int lz, ChunkBlock block);

    void markSectionDirty(int sectionY);
    void markAllDirty();
    // After bulk fill: dirty only sections that contain blocks
    void markNonEmptySectionsDirty();

    // Rebuild only dirty sections (CPU; worker-safe)
    void buildDirtyMeshes(const World& world);

    // Upload any sections with pending GPU data (GL thread)
    void bufferPendingMeshes();

    bool hasMesh() const;
    bool hasPendingUpload() const;
    bool hasDirtySections() const;

    ChunkSection& getSection(int sectionY);
    const ChunkSection& getSection(int sectionY) const;

private:
    static bool validLocalXZ(int lx, int lz);
    static int sectionIndex(int worldY);
    static int localY(int worldY);

    int m_cx;
    int m_cz;
    std::array<std::unique_ptr<ChunkSection>, CHUNK_SECTIONS> m_sections;
};

#endif //MINECRAFT_CHUNK_H
