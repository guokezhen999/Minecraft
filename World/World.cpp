//
// World.cpp – async chunk gen/mesh (two workers), frustum cull, flora LOD
// Vertical columns of ChunkSections
//

#include "World.h"
#include "../Renderer/RenderMaster.h"
#include "../Camera.h"
#include "../Util/Frustum.h"
#include "Block/BlockData.h"
#include "Block/Water.h"
#include "LightEngine.h"
#include "WorldSave.h"

#include <cmath>
#include <algorithm>

World::World(int seed, std::string savePath, int renderDistance)
    : m_generator(seed),
      m_savePath(std::move(savePath)),
      m_renderDistance(std::clamp(renderDistance, 4, 16)),
      m_streamRadius(std::min(STREAM_START_RADIUS, m_renderDistance)) {
    m_genWorker = std::thread(&World::genWorkerLoop, this);
    m_meshWorker = std::thread(&World::meshWorkerLoop, this);
}

World::~World() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_running = false;
    }
    m_queueCv.notify_all();
    if (m_genWorker.joinable())
        m_genWorker.join();
    if (m_meshWorker.joinable())
        m_meshWorker.join();
}

int World::getSeed() const {
    return m_generator.getSeed();
}

void World::setRenderDistance(int distance) {
    const int rd = std::clamp(distance, 4, 16);
    if (rd == m_renderDistance)
        return;
    m_renderDistance = rd;
    if (m_streamRadius > m_renderDistance)
        m_streamRadius = m_renderDistance;
}

bool World::tryLoadColumn(Chunk& chunk, int cx, int cz) {
    if (m_savePath.empty())
        return false;
    if (!WorldSave::loadColumn(m_savePath, cx, cz, chunk))
        return false;
    LightEngine::computeColumn(chunk);
    chunk.markNonEmptySectionsDirty();
    chunk.clearModified();
    return true;
}

void World::flushDirtyColumns() {
    std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
    if (m_savePath.empty())
        return;
    for (auto& [key, chunk] : m_chunks) {
        if (!chunk || !chunk->isModified())
            continue;
        if (WorldSave::saveColumn(m_savePath, chunk->getCX(), chunk->getCZ(), *chunk))
            chunk->clearModified();
    }
}

int World::worldToChunk(int worldCoord) {
    return static_cast<int>(std::floor(static_cast<float>(worldCoord) / CHUNK_SIZE));
}

ChunkBlock World::getBlock(int worldX, int worldY, int worldZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return getBlockLocked(worldX, worldY, worldZ);
}

ChunkBlock World::getBlockLocked(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= WORLD_HEIGHT) {
        return ChunkBlock(BlockId::Air);
    }

    int cx = worldToChunk(worldX);
    int cz = worldToChunk(worldZ);
    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end()) {
        return ChunkBlock(BlockId::Air);
    }

    int lx = worldX - cx * CHUNK_SIZE;
    int lz = worldZ - cz * CHUNK_SIZE;
    return it->second->getBlock(lx, worldY, lz);
}

uint8_t World::getSkyLight(int worldX, int worldY, int worldZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return getSkyLightLocked(worldX, worldY, worldZ);
}

uint8_t World::getBlockLight(int worldX, int worldY, int worldZ) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return getBlockLightLocked(worldX, worldY, worldZ);
}

uint8_t World::getSkyLightLocked(int worldX, int worldY, int worldZ) const {
    if (worldY >= WORLD_HEIGHT)
        return static_cast<uint8_t>(LIGHT_LEVEL_MAX);
    if (worldY < 0)
        return 0;

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end())
        return 0;

    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    return it->second->getSkyLight(lx, worldY, lz);
}

uint8_t World::getBlockLightLocked(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= WORLD_HEIGHT)
        return 0;

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end())
        return 0;

    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    return it->second->getBlockLight(lx, worldY, lz);
}

void World::getLightsLocked(int worldX, int worldY, int worldZ,
                            uint8_t& sky, uint8_t& block) const {
    if (worldY >= WORLD_HEIGHT) {
        sky = static_cast<uint8_t>(LIGHT_LEVEL_MAX);
        block = 0;
        return;
    }
    if (worldY < 0) {
        sky = 0;
        block = 0;
        return;
    }
    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end()) {
        sky = 0;
        block = 0;
        return;
    }
    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    it->second->getLights(lx, worldY, lz, sky, block);
}

void World::setSkyLightLocked(int worldX, int worldY, int worldZ, uint8_t value) {
    if (worldY < 0 || worldY >= WORLD_HEIGHT)
        return;
    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end())
        return;
    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    it->second->setSkyLight(lx, worldY, lz, value);
}

void World::setBlockLightLocked(int worldX, int worldY, int worldZ, uint8_t value) {
    if (worldY < 0 || worldY >= WORLD_HEIGHT)
        return;
    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end())
        return;
    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    it->second->setBlockLight(lx, worldY, lz, value);
}

Chunk* World::getChunkLocked(int cx, int cz) {
    auto it = m_chunks.find(chunkKey(cx, cz));
    return it == m_chunks.end() ? nullptr : it->second.get();
}

const Chunk* World::getChunkLocked(int cx, int cz) const {
    auto it = m_chunks.find(chunkKey(cx, cz));
    return it == m_chunks.end() ? nullptr : it->second.get();
}

void World::advanceTime(int ticks) {
    if (ticks <= 0)
        return;
    m_worldTick += static_cast<uint64_t>(ticks);
}

namespace {

float dayAmountForTick(int t) {
    const int dayStart = TICK_NOON - DAY_PLATEAU_HALF;
    const int dayEnd = TICK_NOON + DAY_PLATEAU_HALF;
    const int nightStart = TICK_MIDNIGHT - NIGHT_PLATEAU_HALF;
    const int nightEnd = TICK_MIDNIGHT + NIGHT_PLATEAU_HALF;
    const float tf = static_cast<float>(t);

    if (t >= dayStart && t < dayEnd)
        return 1.0f;
    if (t >= nightStart && t < nightEnd)
        return 0.0f;
    if (t >= dayEnd && t < nightStart) {
        return 1.0f - glm::smoothstep(static_cast<float>(dayEnd),
                                      static_cast<float>(nightStart), tf);
    }

    const int dawnLen = (DAY_LENGTH - nightEnd) + dayStart;
    const int elapsed =
        (t >= nightEnd) ? (t - nightEnd) : (t + (DAY_LENGTH - nightEnd));
    return glm::smoothstep(0.0f, static_cast<float>(dawnLen),
                           static_cast<float>(elapsed));
}

float twilightBell(int t, int start, int end) {
    int len = 0;
    int elapsed = 0;
    if (end > start) {
        if (t < start || t >= end)
            return 0.0f;
        len = end - start;
        elapsed = t - start;
    } else {
        if (t < start && t >= end)
            return 0.0f;
        len = (DAY_LENGTH - start) + end;
        elapsed = (t >= start) ? (t - start) : (t + DAY_LENGTH - start);
    }
    const float u = static_cast<float>(elapsed) / static_cast<float>(len);
    return std::sin(u * 3.14159265f);
}

} // namespace

Atmosphere World::getAtmosphere() const {
    Atmosphere atmo;
    const int t = static_cast<int>(m_worldTick % static_cast<uint64_t>(DAY_LENGTH));
    const float day01 = dayAmountForTick(t);
    atmo.dayFactor = glm::mix(NIGHT_DAY_FACTOR, 1.0f, day01);

    const int dayEnd = TICK_NOON + DAY_PLATEAU_HALF;
    const int nightStart = TICK_MIDNIGHT - NIGHT_PLATEAU_HALF;
    const int nightEnd = TICK_MIDNIGHT + NIGHT_PLATEAU_HALF;
    const int dayStart = TICK_NOON - DAY_PLATEAU_HALF;
    const float dusk = twilightBell(t, dayEnd, nightStart);
    const float dawn = twilightBell(t, nightEnd, dayStart);

    const glm::vec3 dayTop(SKY_TOP_R, SKY_TOP_G, SKY_TOP_B);
    const glm::vec3 dayHor(SKY_HORIZON_R, SKY_HORIZON_G, SKY_HORIZON_B);
    const glm::vec3 duskTop(0.16f, 0.08f, 0.22f);
    const glm::vec3 duskHor(0.72f, 0.32f, 0.16f);
    const glm::vec3 dawnTop(0.18f, 0.10f, 0.24f);
    const glm::vec3 dawnHor(0.70f, 0.40f, 0.28f);
    const glm::vec3 nightTop(0.03f, 0.04f, 0.10f);
    const glm::vec3 nightHor(0.05f, 0.06f, 0.12f);
    const glm::vec3 nightFog(0.04f, 0.05f, 0.09f);

    atmo.skyTop = glm::mix(nightTop, dayTop, day01);
    atmo.skyHorizon = glm::mix(nightHor, dayHor, day01);
    atmo.skyTop = glm::mix(atmo.skyTop, duskTop, dusk * 0.85f);
    atmo.skyHorizon = glm::mix(atmo.skyHorizon, duskHor, dusk);
    atmo.skyTop = glm::mix(atmo.skyTop, dawnTop, dawn * 0.75f);
    atmo.skyHorizon = glm::mix(atmo.skyHorizon, dawnHor, dawn);
    atmo.fogColor = glm::mix(nightFog, atmo.skyHorizon, day01);
    const float range = static_cast<float>(m_renderDistance * CHUNK_SIZE);
    atmo.fogStart = range * 0.55f;
    atmo.fogEnd = range * 0.92f;
    return atmo;
}

bool World::isChunkLoaded(int cx, int cz) const {
    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    return m_chunks.count(chunkKey(cx, cz)) > 0;
}

int World::getSurfaceHeight(int worldX, int worldZ) const {
    return m_generator.getHeight(worldX, worldZ);
}

bool World::isCollidable(int worldX, int worldY, int worldZ) const {
    if (worldY < 0)
        return true;
    if (worldY >= WORLD_HEIGHT)
        return false;

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);

    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    if (m_chunks.find(chunkKey(cx, cz)) == m_chunks.end())
        return true; // Unloaded: treat as solid so the player does not fall through

    return getBlockLocked(worldX, worldY, worldZ).GetData().isCollidable;
}

bool World::setBlock(int worldX, int worldY, int worldZ, ChunkBlock block) {
    if (worldY < 0 || worldY >= WORLD_HEIGHT)
        return false;

    std::vector<std::pair<int, int>> remeshCols;
    remeshCols.reserve(5);

    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        if (!setBlockDeferred(worldX, worldY, worldZ, block, remeshCols, true))
            return false;
        LightEngine::updateAfterEdit(*this, worldX, worldY, worldZ, remeshCols);
        m_lightDirty.clear();
    }

    remeshAndUploadColumns(remeshCols);
    return true;
}

bool World::setBlockDeferred(int worldX, int worldY, int worldZ, ChunkBlock block,
                             std::vector<std::pair<int, int>>& remeshCols,
                             bool scheduleFluid) {
    if (worldY < 0 || worldY >= WORLD_HEIGHT)
        return false;

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    const int sy = worldY / CHUNK_SIZE;

    auto it = m_chunks.find(chunkKey(cx, cz));
    if (it == m_chunks.end())
        return false;

    if (it->second->getBlock(lx, worldY, lz) == block)
        return false;

    it->second->setBlock(lx, worldY, lz, block);
    noteLightDirty(cx, cz);

    auto addRemesh = [&](int rcx, int rcz) {
        for (const auto& c : remeshCols) {
            if (c.first == rcx && c.second == rcz)
                return;
        }
        remeshCols.emplace_back(rcx, rcz);
    };

    addRemesh(cx, cz);
    if (lx == 0) {
        auto nit = m_chunks.find(chunkKey(cx - 1, cz));
        if (nit != m_chunks.end()) {
            nit->second->markSectionDirty(sy);
            addRemesh(cx - 1, cz);
        }
    }
    if (lx == CHUNK_SIZE - 1) {
        auto nit = m_chunks.find(chunkKey(cx + 1, cz));
        if (nit != m_chunks.end()) {
            nit->second->markSectionDirty(sy);
            addRemesh(cx + 1, cz);
        }
    }
    if (lz == 0) {
        auto nit = m_chunks.find(chunkKey(cx, cz - 1));
        if (nit != m_chunks.end()) {
            nit->second->markSectionDirty(sy);
            addRemesh(cx, cz - 1);
        }
    }
    if (lz == CHUNK_SIZE - 1) {
        auto nit = m_chunks.find(chunkKey(cx, cz + 1));
        if (nit != m_chunks.end()) {
            nit->second->markSectionDirty(sy);
            addRemesh(cx, cz + 1);
        }
    }

    // Vertical neighbor section when on section boundary
    if (worldY % CHUNK_SIZE == 0 && sy > 0)
        it->second->markSectionDirty(sy - 1);
    if (worldY % CHUNK_SIZE == CHUNK_SIZE - 1 && sy + 1 < CHUNK_SECTIONS)
        it->second->markSectionDirty(sy + 1);

    if (scheduleFluid)
        scheduleFluidAround(worldX, worldY, worldZ);

    return true;
}

void World::remeshAndUploadColumns(const std::vector<std::pair<int, int>>& cols) {
    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        for (auto [rcx, rcz] : cols) {
            auto rit = m_chunks.find(chunkKey(rcx, rcz));
            if (rit == m_chunks.end())
                continue;
            rit->second->buildDirtyMeshes(*this);
        }
    }

    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        for (auto [rcx, rcz] : cols) {
            const uint64_t key = chunkKey(rcx, rcz);
            m_meshQueued.erase(key);
            for (auto qit = m_meshQueue.begin(); qit != m_meshQueue.end(); ) {
                if (qit->cx == rcx && qit->cz == rcz)
                    qit = m_meshQueue.erase(qit);
                else
                    ++qit;
            }
        }
    }

    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    for (auto [rcx, rcz] : cols) {
        auto rit = m_chunks.find(chunkKey(rcx, rcz));
        if (rit != m_chunks.end() && rit->second->hasPendingUpload())
            rit->second->bufferPendingMeshes();
    }
}

void World::scheduleFluidUpdate(int x, int y, int z) {
    if (y < 0 || y >= WORLD_HEIGHT)
        return;
    BlockPos p{x, y, z};
    if (m_fluidQueued.count(p))
        return;
    m_fluidQueued.insert(p);
    m_fluidQueue.push_back(p);
}

void World::scheduleFluidAround(int x, int y, int z) {
    scheduleFluidUpdate(x, y, z);
    scheduleFluidUpdate(x + 1, y, z);
    scheduleFluidUpdate(x - 1, y, z);
    scheduleFluidUpdate(x, y + 1, z);
    scheduleFluidUpdate(x, y - 1, z);
    scheduleFluidUpdate(x, y, z + 1);
    scheduleFluidUpdate(x, y, z - 1);
}

void World::updateFluids() {
    if (m_fluidQueue.empty())
        return;

    std::vector<std::pair<int, int>> remeshCols;
    remeshCols.reserve(32);

    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        int budget = MAX_FLUID_UPDATES_PER_FRAME;
        while (budget-- > 0 && !m_fluidQueue.empty()) {
            BlockPos p = m_fluidQueue.front();
            m_fluidQueue.pop_front();
            m_fluidQueued.erase(p);
            updateFluidAt(p.x, p.y, p.z, remeshCols);
        }
        if (!remeshCols.empty())
            flushLightUpdates(remeshCols);
    }

    if (!remeshCols.empty())
        remeshAndUploadColumns(remeshCols);
}

void World::updateFluidAt(int x, int y, int z,
                          std::vector<std::pair<int, int>>& remeshCols) {
    ChunkBlock self = getBlockLocked(x, y, z);

    static const int kDX[4] = {1, -1, 0, 0};
    static const int kDZ[4] = {0, 0, 1, -1};

    auto countHorizontalSources = [&](int cx, int cy, int cz) {
        int n = 0;
        for (int i = 0; i < 4; ++i) {
            if (Water::isSource(getBlockLocked(cx + kDX[i], cy, cz + kDZ[i])))
                ++n;
        }
        return n;
    };

    auto hasInfiniteSupport = [&](int cx, int cy, int cz) {
        if (cy <= 0)
            return true;
        return Water::hasInfiniteSupportBelow(getBlockLocked(cx, cy - 1, cz), cy);
    };

    // Minecraft-style: ≥2 horizontal sources + solid/source below → new source.
    // Only upgrade existing flowing water — never spawn a source straight into
    // air/plants (that made dug cells snap back to water in the same tick).
    auto tryFormInfiniteSource = [&](int cx, int cy, int cz) -> bool {
        if (!hasInfiniteSupport(cx, cy, cz))
            return false;
        if (countHorizontalSources(cx, cy, cz) < 2)
            return false;
        ChunkBlock here = getBlockLocked(cx, cy, cz);
        if (Water::isSource(here))
            return true;
        if (!Water::isWater(here))
            return false;
        setBlockDeferred(cx, cy, cz, Water::makeSource(), remeshCols, true);
        return true;
    };

    auto tryFlowInto = [&](int nx, int ny, int nz, int newLevel) {
        if (ny < 0 || ny >= WORLD_HEIGHT)
            return;
        ChunkBlock dest = getBlockLocked(nx, ny, nz);
        if (!Water::canFlowInto(dest))
            return;

        // Prefer creating an infinite source when the 2-source rule matches
        if (tryFormInfiniteSource(nx, ny, nz))
            return;

        if (Water::isWater(dest) && Water::level(dest) <= newLevel)
            return; // already same or deeper / closer to source
        setBlockDeferred(nx, ny, nz, Water::make(newLevel), remeshCols, true);
    };

    if (Water::isWater(self)) {
        // Upgrade flowing water → source when two sources hug this cell
        if (!Water::isSource(self))
            tryFormInfiniteSource(x, y, z);

        self = getBlockLocked(x, y, z);
        if (!Water::isWater(self))
            return;

        const int curLevel = Water::level(self);

        const ChunkBlock below =
            (y > 0) ? getBlockLocked(x, y - 1, z) : ChunkBlock(BlockId::Stone);
        const bool onGround = Water::isOnSolidGround(below, y);
        const bool canFall = (y > 0) && Water::canFlowInto(below);

        if (canFall) {
            // Mid-air / above fluid: fall only — never spread sideways
            const int downLevel = 1;
            if (!Water::isWater(below) || Water::level(below) > downLevel)
                tryFlowInto(x, y - 1, z, downLevel);
        } else if (onGround && curLevel < Water::MAX_LEVEL) {
            // Only the cell resting on solid ground may push sideways,
            // and only into ground-level cells (or empty ledge that will fall).
            for (int i = 0; i < 4; ++i) {
                const int nx = x + kDX[i];
                const int nz = z + kDZ[i];
                if (y > 0) {
                    const ChunkBlock destBelow = getBlockLocked(nx, y - 1, nz);
                    const bool destOnGround = Water::isOnSolidGround(destBelow, y);
                    const bool destOpenBelow =
                        destBelow == BlockId::Air ||
                        destBelow.GetData().meshType == BlockMeshType::X;
                    // No sideways fill into cells sitting on water / mid column
                    if (!destOnGround && !destOpenBelow)
                        continue;
                }
                tryFlowInto(nx, y, nz, curLevel + 1);
            }
        }

        // Non-source: recompute level from neighbors or evaporate
        if (!Water::isSource(getBlockLocked(x, y, z))) {
            int best = Water::MAX_LEVEL + 1;
            if (y + 1 < WORLD_HEIGHT && Water::isWater(getBlockLocked(x, y + 1, z)))
                best = 0;

            for (int i = 0; i < 4; ++i) {
                ChunkBlock n = getBlockLocked(x + kDX[i], y, z + kDZ[i]);
                if (Water::isWater(n))
                    best = std::min(best, Water::level(n));
            }

            const int newLevel = best + 1;
            if (tryFormInfiniteSource(x, y, z))
                return;

            if (newLevel > Water::MAX_LEVEL) {
                setBlockDeferred(x, y, z, ChunkBlock(BlockId::Air), remeshCols, true);
            } else if (newLevel != Water::level(getBlockLocked(x, y, z))) {
                setBlockDeferred(x, y, z, Water::make(newLevel), remeshCols, true);
            }
        }
        return;
    }

    // Empty / plant cell: infinite source, or fall-from-above only.
    // Do NOT pull from horizontal neighbors — that bypassed the "ground only" rule.
    if (!Water::canFlowInto(self))
        return;

    if (tryFormInfiniteSource(x, y, z))
        return;

    if (y + 1 < WORLD_HEIGHT && Water::isWater(getBlockLocked(x, y + 1, z)))
        tryFlowInto(x, y, z, 1);
}

void World::setWorldBlock(Chunk& chunk, int cx, int cz,
                           int wx, int wy, int wz, BlockId id) {
    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    if (lx < 0 || lx >= CHUNK_SIZE) return;
    if (lz < 0 || lz >= CHUNK_SIZE) return;
    if (wy < 0 || wy >= WORLD_HEIGHT) return;
    chunk.setBlockRaw(lx, wy, lz, ChunkBlock(id));
}

void World::setDecorationBlock(Chunk& chunk, int cx, int cz,
                                int wx, int wy, int wz, BlockId id) {
    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    if (lx < 0 || lx >= CHUNK_SIZE) return;
    if (lz < 0 || lz >= CHUNK_SIZE) return;
    if (wy < 0 || wy >= WORLD_HEIGHT) return;
    // Do not plant into water or replace existing solid blocks
    if (chunk.getBlock(lx, wy, lz) != BlockId::Air)
        return;
    chunk.setBlockRaw(lx, wy, lz, ChunkBlock(id));
}

void World::placeOakTree(Chunk& chunk, int cx, int cz,
                          int wx, int surfY, int wz, int trunkHeight,
                          BlockId bark, BlockId leaf) {
    for (int ty = surfY + 1; ty <= surfY + trunkHeight; ++ty) {
        setDecorationBlock(chunk, cx, cz, wx, ty, wz, bark);
    }

    int topY = surfY + trunkHeight;

    struct Layer { int dy; int radius; bool trimCorners; };
    Layer layers[] = {
        { 2,  1, false },
        { 1,  2, true  },
        { 0,  2, true  },
        {-1,  1, false },
    };

    for (auto& layer : layers) {
        int ly = topY + layer.dy;
        int r  = layer.radius;
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                if (layer.trimCorners && std::abs(dx) == r && std::abs(dz) == r)
                    continue;
                if (dx == 0 && dz == 0 && ly <= topY && ly > surfY)
                    continue;
                setDecorationBlock(chunk, cx, cz,
                                   wx + dx, ly, wz + dz, leaf);
            }
        }
    }
}

void World::placeCactus(Chunk& chunk, int cx, int cz, int wx, int surfY, int wz) {
    int h = ((wx * 1234567) ^ (wz * 7654321)) & 0x3;
    h = std::max(1, h);
    for (int ty = surfY + 1; ty <= surfY + h; ++ty) {
        setDecorationBlock(chunk, cx, cz, wx, ty, wz, BlockId::Cactus);
    }
}

void World::placeSpruceTree(Chunk& chunk, int cx, int cz,
                             int wx, int surfY, int wz, int trunkHeight) {
    for (int ty = surfY + 1; ty <= surfY + trunkHeight; ++ty) {
        setDecorationBlock(chunk, cx, cz, wx, ty, wz, BlockId::SpruceBark);
    }
    int topY = surfY + trunkHeight;
    
    // Top tip leaf
    setDecorationBlock(chunk, cx, cz, wx, topY + 1, wz, BlockId::SpruceLeaf);
    
    // Pine cone leaf layers
    for (int dy = 0; dy >= -trunkHeight + 2; --dy) {
        int ly = topY + dy;
        int r = (dy % 2 == 0) ? 1 : 2;
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                if (r == 2 && std::abs(dx) == 2 && std::abs(dz) == 2)
                    continue; // Trim corners
                if (dx == 0 && dz == 0 && ly <= topY)
                    continue;
                setDecorationBlock(chunk, cx, cz, wx + dx, ly, wz + dz, BlockId::SpruceLeaf);
            }
        }
    }
}

void World::placeJungleTree(Chunk& chunk, int cx, int cz,
                             int wx, int surfY, int wz, int trunkHeight) {
    for (int ty = surfY + 1; ty <= surfY + trunkHeight; ++ty) {
        setDecorationBlock(chunk, cx, cz, wx, ty, wz, BlockId::JungleBark);
    }
    int topY = surfY + trunkHeight;
    
    // Spherical lush leaf dome
    for (int dy = -2; dy <= 2; ++dy) {
        int ly = topY + dy;
        int r = 3 - std::abs(dy);
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                if (r >= 2 && std::abs(dx) == r && std::abs(dz) == r)
                    continue; // Trim corners
                if (dx == 0 && dz == 0 && ly <= topY)
                    continue;
                setDecorationBlock(chunk, cx, cz, wx + dx, ly, wz + dz, BlockId::JungleLeaf);
            }
        }
    }
}

void World::placeDecorations(Chunk& chunk, int cx, int cz,
                             const TerrainColumn interior[CHUNK_SIZE][CHUNK_SIZE]) {
    const int TREE_CANOPY_RADIUS = 3;
    const int wxMin = cx * CHUNK_SIZE - TREE_CANOPY_RADIUS;
    const int wxMax = cx * CHUNK_SIZE + CHUNK_SIZE - 1 + TREE_CANOPY_RADIUS;
    const int wzMin = cz * CHUNK_SIZE - TREE_CANOPY_RADIUS;
    const int wzMax = cz * CHUNK_SIZE + CHUNK_SIZE - 1 + TREE_CANOPY_RADIUS;

    for (int wx = wxMin; wx <= wxMax; ++wx) {
        for (int wz = wzMin; wz <= wzMax; ++wz) {
            const int lx = wx - cx * CHUNK_SIZE;
            const int lz = wz - cz * CHUNK_SIZE;
            const TerrainColumn col =
                (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE)
                    ? interior[lx][lz]
                    : m_generator.sampleColumn(wx, wz);
            const int surfY = col.height;
            const BiomeType biome = col.biome;

            // Trees and plants stay above the water line
            if (surfY <= WATER_LEVEL)
                continue;

            const BlockId surface = static_cast<BlockId>(
                m_generator.getBlock(wx, surfY, wz, surfY, biome));
            const float wet = TerrainGenerator::wetFactor(col.moisture);

            if (m_generator.shouldPlaceTree(wx, wz, col)) {
                int trunkH = m_generator.getTreeHeight(wx, wz, biome);
                const int maxTrunk = WORLD_HEIGHT - surfY - 4;
                if (maxTrunk < 3)
                    continue;
                trunkH = std::min(trunkH, maxTrunk);
                if (biome == BiomeType::Savanna) {
                    placeOakTree(chunk, cx, cz, wx, surfY, wz, trunkH,
                                 BlockId::SavannaBark, BlockId::SavannaLeaf);
                } else if (biome == BiomeType::Taiga) {
                    placeSpruceTree(chunk, cx, cz, wx, surfY, wz, trunkH);
                } else if (biome == BiomeType::Jungle) {
                    placeJungleTree(chunk, cx, cz, wx, surfY, wz, trunkH);
                } else {
                    placeOakTree(chunk, cx, cz, wx, surfY, wz, trunkH,
                                 BlockId::OakBark, BlockId::OakLeaf);
                }
                continue;
            }

            if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE)
                continue;

            // Cactus / dead shrub: desert edges only (deep desert stays barren)
            if (surface == BlockId::Sand && biome == BiomeType::Desert
                && col.moisture >= DEEP_DESERT_MOISTURE) {
                if (m_generator.columnRoll(wx, wz, 610u) < (1.0f / 64.0f) * wet) {
                    placeCactus(chunk, cx, cz, wx, surfY, wz);
                } else if (m_generator.columnRoll(wx, wz, 611u) < (1.0f / 32.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::DeadShrub);
                }
                continue;
            }

            // Gobi (TemperateDesert): Dead Shrubs only (no cacti)
            if (biome == BiomeType::TemperateDesert &&
                (surface == BlockId::Sand || surface == BlockId::Stone)) {
                if (m_generator.columnRoll(wx, wz, 611u) < (1.0f / 30.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::DeadShrub);
                }
                continue;
            }

            // Tundra: sedge/moss tufts on TundraGrass; dwarf shrubs on moss or leftover snow
            if (biome == BiomeType::Tundra &&
                (surface == BlockId::TundraGrass || surface == BlockId::Snow)) {
                if (surface == BlockId::TundraGrass &&
                    m_generator.columnRoll(wx, wz, 660u) < (1.0f / 14.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::Fern);
                } else if (m_generator.columnRoll(wx, wz, 661u) < (1.0f / 40.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::DeadShrub);
                }
                continue;
            }

            // Forest
            if (biome == BiomeType::Forest && surface == BlockId::Grass) {
                if (m_generator.columnRoll(wx, wz, 620u) < (1.0f / 28.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::TallGrass);
                } else if (m_generator.columnRoll(wx, wz, 621u) < (1.0f / 120.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::Rose);
                }
            }
            // Grassland (Temperate Grassland)
            else if (biome == BiomeType::Grassland && surface == BlockId::Grass) {
                if (m_generator.columnRoll(wx, wz, 620u) < (1.0f / 15.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::TallGrass);
                } else if (m_generator.columnRoll(wx, wz, 621u) < (1.0f / 60.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::Rose);
                }
            }
            // Savanna
            else if (biome == BiomeType::Savanna && surface == BlockId::SavannaGrass) {
                if (m_generator.columnRoll(wx, wz, 630u) < (1.0f / 48.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::SavannaTallGrass);
                }
            }
            // Taiga
            else if (biome == BiomeType::Taiga && surface == BlockId::TaigaGrass) {
                if (m_generator.columnRoll(wx, wz, 640u) < (1.0f / 20.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::Fern);
                } else if (m_generator.columnRoll(wx, wz, 641u) < (1.0f / 40.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::DeadShrub);
                }
            }
            // Jungle
            else if (biome == BiomeType::Jungle && surface == BlockId::Grass) {
                if (m_generator.columnRoll(wx, wz, 650u) < (1.0f / 12.0f) * wet) {
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::Fern);
                }
            }
        }
    }
}

void World::fillChunk(Chunk& chunk, int cx, int cz) {
    TerrainColumn interior[CHUNK_SIZE][CHUNK_SIZE];
    for (int bx = 0; bx < CHUNK_SIZE; ++bx) {
        for (int bz = 0; bz < CHUNK_SIZE; ++bz) {
            const int worldX = cx * CHUNK_SIZE + bx;
            const int worldZ = cz * CHUNK_SIZE + bz;

            // Height / biome once per column (not once per Y)
            const TerrainColumn col = m_generator.sampleColumn(worldX, worldZ);
            interior[bx][bz] = col;
            const int height = col.height;
            const BiomeType biome = col.biome;

            // Only iterate solid + water range; air above sea/surface is default
            const int yMax = std::max(height, WATER_LEVEL);
            for (int by = 0; by <= yMax; ++by) {
                const int blockType =
                    m_generator.getBlock(worldX, by, worldZ, height, biome);
                if (blockType != static_cast<int>(BlockId::Air)) {
                    chunk.setBlockRaw(bx, by, bz,
                                      ChunkBlock(static_cast<BlockId>(blockType)));
                }
            }
        }
    }
    placeDecorations(chunk, cx, cz, interior);
    LightEngine::computeColumn(chunk);
    // Skip empty upper sections — they stay clean and never mesh
    chunk.markNonEmptySectionsDirty();
}

void World::markMeshDirty(int cx, int cz) {
    uint64_t key = chunkKey(cx, cz);
    {
        std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
        if (!m_chunks.count(key)) return;
    }

    std::lock_guard<std::mutex> qlock(m_queueMutex);
    if (m_meshQueued.insert(key).second) {
        m_meshQueue.push_back({cx, cz});
        m_queueCv.notify_all();
    }
}

void World::markNeighborsDirty(int cx, int cz) {
    // Shared-face cull only: remesh neighbor sections that actually touch this column.
    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        auto selfIt = m_chunks.find(chunkKey(cx, cz));
        const Chunk* self = (selfIt != m_chunks.end()) ? selfIt->second.get() : nullptr;

        const std::pair<int, int> neighbors[] = {
            {cx + 1, cz}, {cx - 1, cz}, {cx, cz + 1}, {cx, cz - 1}
        };
        for (auto [ncx, ncz] : neighbors) {
            auto it = m_chunks.find(chunkKey(ncx, ncz));
            if (it == m_chunks.end())
                continue;
            Chunk& neighbor = *it->second;
            for (int sy = 0; sy < CHUNK_SECTIONS; ++sy) {
                const bool neighborHas = !neighbor.getSection(sy).isEmpty();
                const bool selfHas = self && !self->getSection(sy).isEmpty();
                if (neighborHas && selfHas)
                    neighbor.markSectionDirty(sy);
            }
        }
    }
    markMeshDirty(cx + 1, cz);
    markMeshDirty(cx - 1, cz);
    markMeshDirty(cx, cz + 1);
    markMeshDirty(cx, cz - 1);
}

void World::enqueueMissingChunks(int centerCX, int centerCZ) {
    while (m_streamRadius < m_renderDistance &&
           isRingClaimed(centerCX, centerCZ, m_streamRadius)) {
        ++m_streamRadius;
    }

    struct Candidate { int cx, cz, dist2; };
    std::vector<Candidate> missing;
    const int r = m_streamRadius;
    missing.reserve((r * 2 + 1) * (r * 2 + 1));

    {
        std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                const int cx = centerCX + dx;
                const int cz = centerCZ + dz;
                if (m_chunks.count(chunkKey(cx, cz)))
                    continue;
                missing.push_back({cx, cz, dx * dx + dz * dz});
            }
        }
    }

    if (missing.empty())
        return;

    std::sort(missing.begin(), missing.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.dist2 < b.dist2;
              });

    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        bool added = false;
        for (const auto& c : missing) {
            uint64_t key = chunkKey(c.cx, c.cz);
            if (m_genQueued.count(key)) continue;
            m_genQueued.insert(key);
            m_genQueue.push_back({c.cx, c.cz});
            added = true;
        }
        if (added)
            m_queueCv.notify_all();
    }
}

bool World::isRingClaimed(int centerCX, int centerCZ, int radius) const {
    std::vector<uint64_t> missing;
    {
        std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                const uint64_t key = chunkKey(centerCX + dx, centerCZ + dz);
                if (!m_chunks.count(key))
                    missing.push_back(key);
            }
        }
    }
    if (missing.empty())
        return true;
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (uint64_t key : missing) {
        if (!m_genQueued.count(key))
            return false;
    }
    return true;
}

bool World::isStreaming() const {
    if (m_streamRadius < m_renderDistance)
        return true;
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return !m_genQueue.empty() || !m_generatedChunks.empty()
        || !m_meshQueue.empty() || !m_uploadQueue.empty();
}

void World::integrateGeneratedChunks(int budget) {
    std::deque<std::unique_ptr<Chunk>> ready;
    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        int n = 0;
        while (!m_generatedChunks.empty() && n < budget) {
            ready.push_back(std::move(m_generatedChunks.front()));
            m_generatedChunks.pop_front();
            ++n;
        }
    }

    if (ready.empty()) return;

    std::vector<std::pair<int, int>> inserted;
    inserted.reserve(ready.size());
    std::vector<uint64_t> readyKeys;
    readyKeys.reserve(ready.size());
    std::vector<std::pair<int, int>> lightRemesh;

    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        for (auto& chunk : ready) {
            const int cx = chunk->getCX();
            const int cz = chunk->getCZ();
            const uint64_t key = chunkKey(cx, cz);
            readyKeys.push_back(key);
            if (m_chunks.count(key)) continue;
            m_chunks[key] = std::move(chunk);
            inserted.emplace_back(cx, cz);
        }
        for (auto [cx, cz] : inserted)
            LightEngine::propagateFromNeighbors(*this, cx, cz, lightRemesh);
    }

    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        for (uint64_t key : readyKeys)
            m_genQueued.erase(key);
    }

    for (auto [cx, cz] : inserted) {
        markMeshDirty(cx, cz);
        markNeighborsDirty(cx, cz);
    }
    for (auto [rcx, rcz] : lightRemesh)
        markMeshDirty(rcx, rcz);
}

void World::processMeshUploads(int budget) {
    std::deque<ChunkCoord> uploads;
    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        int n = 0;
        while (!m_uploadQueue.empty() && n < budget) {
            uploads.push_back(m_uploadQueue.front());
            m_uploadQueue.pop_front();
            ++n;
        }
    }

    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    for (const auto& c : uploads) {
        auto it = m_chunks.find(chunkKey(c.cx, c.cz));
        if (it == m_chunks.end()) continue;
        if (it->second->hasPendingUpload()) {
            it->second->bufferPendingMeshes();
        }
    }
}

void World::unloadDistantChunks(int centerCX, int centerCZ) {
    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
            const int dx = it->second->getCX() - centerCX;
            const int dz = it->second->getCZ() - centerCZ;
            if (std::abs(dx) > unloadDistance() || std::abs(dz) > unloadDistance()) {
                if (it->second->isModified() && !m_savePath.empty()) {
                    if (!WorldSave::saveColumn(m_savePath, it->second->getCX(),
                                               it->second->getCZ(), *it->second)) {
                        ++it;
                        continue;
                    }
                    it->second->clearModified();
                }
                it = m_chunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::lock_guard<std::mutex> qlock(m_queueMutex);
    auto dropFar = [&](std::deque<ChunkCoord>& q, std::unordered_set<uint64_t>* set,
                       int maxDist) {
        for (auto it = q.begin(); it != q.end(); ) {
            int dx = it->cx - centerCX;
            int dz = it->cz - centerCZ;
            if (std::abs(dx) > maxDist || std::abs(dz) > maxDist) {
                if (set) set->erase(chunkKey(it->cx, it->cz));
                it = q.erase(it);
            } else {
                ++it;
            }
        }
    };
    dropFar(m_genQueue, &m_genQueued, m_renderDistance);
    dropFar(m_meshQueue, &m_meshQueued, unloadDistance());
    dropFar(m_uploadQueue, nullptr, unloadDistance());
}

void World::genWorkerLoop() {
    while (true) {
        ChunkCoord genTask{};
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [&] {
                return !m_running || !m_genQueue.empty();
            });
            if (!m_running && m_genQueue.empty())
                return;
            if (m_genQueue.empty())
                continue;
            genTask = m_genQueue.front();
            m_genQueue.pop_front();
            // Keep m_genQueued until integrate so the same column is not enqueued twice.
        }

        auto chunk = std::make_unique<Chunk>(genTask.cx, genTask.cz);
        if (!tryLoadColumn(*chunk, genTask.cx, genTask.cz))
            fillChunk(*chunk, genTask.cx, genTask.cz);
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_generatedChunks.push_back(std::move(chunk));
        }
    }
}

void World::meshWorkerLoop() {
    while (true) {
        ChunkCoord meshTask{};
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [&] {
                return !m_running || !m_meshQueue.empty();
            });
            if (!m_running && m_meshQueue.empty())
                return;
            if (m_meshQueue.empty())
                continue;
            meshTask = m_meshQueue.front();
            m_meshQueue.pop_front();
            m_meshQueued.erase(chunkKey(meshTask.cx, meshTask.cz));
        }

        {
            std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
            auto it = m_chunks.find(chunkKey(meshTask.cx, meshTask.cz));
            if (it == m_chunks.end())
                continue;
            // buildDirtyMeshes only reads blocks + neighbors via getBlock
            it->second->buildDirtyMeshes(*this);
        }
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_uploadQueue.push_back(meshTask);
        }
    }
}

void World::Update(const glm::vec3& cameraPos) {
    int centerCX = worldToChunk(static_cast<int>(std::floor(cameraPos.x)));
    int centerCZ = worldToChunk(static_cast<int>(std::floor(cameraPos.z)));

    unloadDistantChunks(centerCX, centerCZ);
    enqueueMissingChunks(centerCX, centerCZ);
    const bool streaming = isStreaming();
    integrateGeneratedChunks(streaming ? MAX_CHUNKS_INTEGRATE_STREAMING
                                       : MAX_CHUNKS_INTEGRATE_PER_FRAME);
    processMeshUploads(streaming ? MAX_MESH_UPLOADS_STREAMING
                                 : MAX_MESH_UPLOADS_PER_FRAME);
    updateFluids();
}

void World::Render(RenderMaster& master, const Camera& camera, bool underwater) {
    const Atmosphere atmo = getAtmosphere();
    Frustum frustum;
    frustum.update(camera.GetProjectionViewMatrix());

    const float fogCull = atmo.fogEnd + static_cast<float>(CHUNK_SIZE) * 1.5f;
    const float renderDistSq = fogCull * fogCull;
    const float floraDist = static_cast<float>(m_renderDistance * CHUNK_SIZE) * 0.55f;
    const float floraDistSq = floraDist * floraDist;
    const glm::vec3 camPos = camera.Position;

    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);

    for (auto& [key, chunk] : m_chunks) {
        if (!chunk->hasMesh()) continue;

        const int cx = chunk->getCX();
        const int cz = chunk->getCZ();
        if (!frustum.intersectsChunkColumn(cx, cz, CHUNK_SIZE, WORLD_HEIGHT))
            continue;

        const float chunkCX = (cx + 0.5f) * CHUNK_SIZE;
        const float chunkCZ = (cz + 0.5f) * CHUNK_SIZE;
        const float dx = chunkCX - camPos.x;
        const float dz = chunkCZ - camPos.z;
        const float distSq = dx * dx + dz * dz;

        if (distSq > renderDistSq) continue;

        const bool drawFlora = distSq <= floraDistSq;

        for (int sy = 0; sy < CHUNK_SECTIONS; ++sy) {
            const ChunkSection& section = chunk->getSection(sy);
            if (!section.hasMesh() || section.isEmpty())
                continue;
            if (!frustum.intersectsChunkSection(cx, sy, cz, CHUNK_SIZE))
                continue;
            master.DrawChunk(section.getMeshes(), distSq, drawFlora);
        }
    }

    lock.unlock();
    master.FinishChunkRender(camera, underwater, atmo);
}

bool World::isCameraUnderwater(const glm::vec3& eyePos) const {
    const int bx = static_cast<int>(std::floor(eyePos.x));
    const int by = static_cast<int>(std::floor(eyePos.y));
    const int bz = static_cast<int>(std::floor(eyePos.z));
    const ChunkBlock here = getBlock(bx, by, bz);
    const ChunkBlock above = getBlock(bx, by + 1, bz);
    return Water::isSubmerged(here, above, eyePos.y);
}

void World::noteLightDirty(int cx, int cz) {
    m_lightDirty.insert(chunkKey(cx, cz));
}

void World::flushLightUpdates(std::vector<std::pair<int, int>>& remeshCols) {
    if (m_lightDirty.empty())
        return;
    std::vector<std::pair<int, int>> cols;
    cols.reserve(m_lightDirty.size());
    for (uint64_t key : m_lightDirty) {
        const int cx = static_cast<int>(static_cast<uint32_t>(key >> 32));
        const int cz = static_cast<int>(static_cast<uint32_t>(key));
        cols.emplace_back(cx, cz);
    }
    m_lightDirty.clear();
    LightEngine::relightColumns(*this, cols, remeshCols);
}
