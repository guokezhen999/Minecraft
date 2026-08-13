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

uint8_t Chunk::getSkyLight(int lx, int worldY, int lz) const {
    if (!validLocalXZ(lx, lz) || worldY < 0)
        return 0;
    if (worldY >= WORLD_HEIGHT)
        return static_cast<uint8_t>(LIGHT_LEVEL_MAX);
    return m_sections[sectionIndex(worldY)]->skyLightRaw(lx, localY(worldY), lz);
}

uint8_t Chunk::getBlockLight(int lx, int worldY, int lz) const {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return 0;
    return m_sections[sectionIndex(worldY)]->blockLightRaw(lx, localY(worldY), lz);
}

void Chunk::setSkyLight(int lx, int worldY, int lz, uint8_t value) {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return;
    m_sections[sectionIndex(worldY)]->setSkyLightRaw(lx, localY(worldY), lz, value);
}

void Chunk::setBlockLight(int lx, int worldY, int lz, uint8_t value) {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return;
    m_sections[sectionIndex(worldY)]->setBlockLightRaw(lx, localY(worldY), lz, value);
}

void Chunk::getLights(int lx, int worldY, int lz, uint8_t& sky, uint8_t& block) const {
    if (!validLocalXZ(lx, lz) || worldY < 0) {
        sky = 0;
        block = 0;
        return;
    }
    if (worldY >= WORLD_HEIGHT) {
        sky = static_cast<uint8_t>(LIGHT_LEVEL_MAX);
        block = 0;
        return;
    }
    const ChunkSection& s = *m_sections[sectionIndex(worldY)];
    const int ly = localY(worldY);
    sky = s.skyLightRaw(lx, ly, lz);
    block = s.blockLightRaw(lx, ly, lz);
}

void Chunk::setBlock(int lx, int worldY, int lz, ChunkBlock block) {
    if (!validLocalXZ(lx, lz) || worldY < 0 || worldY >= WORLD_HEIGHT)
        return;

    const int sy = sectionIndex(worldY);
    const int ly = localY(worldY);
    m_sections[sy]->setBlock(lx, ly, lz, block);
    m_sections[sy]->markDirty();
    m_modified = true;

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
