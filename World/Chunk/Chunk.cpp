//
// Chunk.cpp – vertical column of sections
//

#include "Chunk.h"
#include "../World.h"

Chunk::Chunk(int cx, int cz) : m_cx(cx), m_cz(cz) {
    for (int sy = 0; sy < CHUNK_SECTIONS; ++sy) {
        m_sections[sy] = std::make_unique<ChunkSection>(glm::ivec3(cx, sy, cz));
    }
}

bool Chunk::validLocalXZ(int lx, int lz) {
    return lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE;
}

int Chunk::sectionIndex(int worldY) {
    return worldY / CHUNK_SIZE;
}

int Chunk::localY(int worldY) {
    return worldY % CHUNK_SIZE;
}

ChunkBlock Chunk::getBlock(int lx, int worldY, int lz) const {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return ChunkBlock(BlockId::Air);
    return m_sections[sectionIndex(worldY)]->getBlock(lx, localY(worldY), lz);
}

void Chunk::setBlock(int lx, int worldY, int lz, ChunkBlock block) {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return;

    const int sy = sectionIndex(worldY);
    const int ly = localY(worldY);
    m_sections[sy]->setBlock(lx, ly, lz, block);
    m_sections[sy]->markDirty();

    // Vertical neighbor faces may change
    if (ly == 0 && sy > 0)
        m_sections[sy - 1]->markDirty();
    if (ly == CHUNK_SIZE - 1 && sy + 1 < CHUNK_SECTIONS)
        m_sections[sy + 1]->markDirty();
}

void Chunk::setBlockRaw(int lx, int worldY, int lz, ChunkBlock block) {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return;
    m_sections[sectionIndex(worldY)]->setBlock(lx, localY(worldY), lz, block);
}

void Chunk::markSectionDirty(int sectionY) {
    if (sectionY < 0 || sectionY >= CHUNK_SECTIONS)
        return;
    m_sections[sectionY]->markDirty();
}

void Chunk::markAllDirty() {
    for (auto& section : m_sections)
        section->markDirty();
}

void Chunk::markNonEmptySectionsDirty() {
    for (auto& section : m_sections) {
        if (!section->isEmpty())
            section->markDirty();
    }
}

void Chunk::buildDirtyMeshes(const World& world) {
    for (auto& section : m_sections) {
        if (!section->isDirty())
            continue;
        // Never-meshed empty sections: clear dirty and skip (no GPU work)
        if (section->isEmpty() && !section->hasMesh()) {
            section->clearDirty();
            continue;
        }
        section->buildMesh(world);
    }
}

void Chunk::bufferPendingMeshes() {
    for (auto& section : m_sections) {
        if (section->hasPendingUpload())
            section->bufferMeshes();
    }
}

bool Chunk::hasMesh() const {
    for (const auto& section : m_sections) {
        if (section->hasMesh())
            return true;
    }
    return false;
}

bool Chunk::hasPendingUpload() const {
    for (const auto& section : m_sections) {
        if (section->hasPendingUpload())
            return true;
    }
    return false;
}

bool Chunk::hasDirtySections() const {
    for (const auto& section : m_sections) {
        if (section->isDirty())
            return true;
    }
    return false;
}

ChunkSection& Chunk::getSection(int sectionY) {
    return *m_sections[sectionY];
}

const ChunkSection& Chunk::getSection(int sectionY) const {
    return *m_sections[sectionY];
}
