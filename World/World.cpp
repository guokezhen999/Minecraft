//
// World.cpp – async chunk gen/mesh, frustum cull, flora LOD
// Vertical columns of ChunkSections
//

#include "World.h"
#include "../Renderer/RenderMaster.h"
#include "../Camera.h"
#include "../Util/Frustum.h"
#include "Block/BlockData.h"

#include <cmath>
#include <algorithm>

World::World(int seed) : m_generator(seed) {
    m_worker = std::thread(&World::workerLoop, this);
}

World::~World() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_running = false;
    }
    m_queueCv.notify_all();
    if (m_worker.joinable())
        m_worker.join();
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

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;
    const int sy = worldY / CHUNK_SIZE;

    std::vector<std::pair<int, int>> toRemesh;
    toRemesh.reserve(5);

    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        auto it = m_chunks.find(chunkKey(cx, cz));
        if (it == m_chunks.end())
            return false;

        if (it->second->getBlock(lx, worldY, lz) == block)
            return false;

        it->second->setBlock(lx, worldY, lz, block);

        toRemesh.emplace_back(cx, cz);
        if (lx == 0) {
            auto nit = m_chunks.find(chunkKey(cx - 1, cz));
            if (nit != m_chunks.end()) {
                nit->second->markSectionDirty(sy);
                toRemesh.emplace_back(cx - 1, cz);
            }
        }
        if (lx == CHUNK_SIZE - 1) {
            auto nit = m_chunks.find(chunkKey(cx + 1, cz));
            if (nit != m_chunks.end()) {
                nit->second->markSectionDirty(sy);
                toRemesh.emplace_back(cx + 1, cz);
            }
        }
        if (lz == 0) {
            auto nit = m_chunks.find(chunkKey(cx, cz - 1));
            if (nit != m_chunks.end()) {
                nit->second->markSectionDirty(sy);
                toRemesh.emplace_back(cx, cz - 1);
            }
        }
        if (lz == CHUNK_SIZE - 1) {
            auto nit = m_chunks.find(chunkKey(cx, cz + 1));
            if (nit != m_chunks.end()) {
                nit->second->markSectionDirty(sy);
                toRemesh.emplace_back(cx, cz + 1);
            }
        }

        // Sync remesh dirty sections only so dig/place is visible immediately
        for (auto [rcx, rcz] : toRemesh) {
            auto rit = m_chunks.find(chunkKey(rcx, rcz));
            if (rit == m_chunks.end())
                continue;
            rit->second->buildDirtyMeshes(*this);
        }
    }

    // Drop any pending async remesh for these columns (we already rebuilt)
    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        for (auto [rcx, rcz] : toRemesh) {
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
    for (auto [rcx, rcz] : toRemesh) {
        auto rit = m_chunks.find(chunkKey(rcx, rcz));
        if (rit != m_chunks.end() && rit->second->hasPendingUpload())
            rit->second->bufferPendingMeshes();
    }
    return true;
}

void World::setWorldBlock(Chunk& chunk, int cx, int cz,
                           int wx, int wy, int wz, BlockId id) {
    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    if (lx < 0 || lx >= CHUNK_SIZE) return;
    if (lz < 0 || lz >= CHUNK_SIZE) return;
    if (wy < 0 || wy >= WORLD_HEIGHT) return;
    chunk.setBlock(lx, wy, lz, ChunkBlock(id));
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
    chunk.setBlock(lx, wy, lz, ChunkBlock(id));
}

void World::placeOakTree(Chunk& chunk, int cx, int cz,
                          int wx, int surfY, int wz, int trunkHeight) {
    for (int ty = surfY + 1; ty <= surfY + trunkHeight; ++ty) {
        setDecorationBlock(chunk, cx, cz, wx, ty, wz, BlockId::OakBark);
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
                                   wx + dx, ly, wz + dz, BlockId::OakLeaf);
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

void World::placeDecorations(Chunk& chunk, int cx, int cz) {
    const int TREE_CANOPY_RADIUS = 3;
    const int wxMin = cx * CHUNK_SIZE - TREE_CANOPY_RADIUS;
    const int wxMax = cx * CHUNK_SIZE + CHUNK_SIZE - 1 + TREE_CANOPY_RADIUS;
    const int wzMin = cz * CHUNK_SIZE - TREE_CANOPY_RADIUS;
    const int wzMax = cz * CHUNK_SIZE + CHUNK_SIZE - 1 + TREE_CANOPY_RADIUS;

    for (int wx = wxMin; wx <= wxMax; ++wx) {
        for (int wz = wzMin; wz <= wzMax; ++wz) {
            const BiomeType biome = m_generator.getBiome(wx, wz);
            const int surfY = m_generator.getHeight(wx, wz);

            // Surface at or below sea level is flooded — no plants
            if (surfY < WATER_LEVEL)
                continue;

            const BlockId surface = static_cast<BlockId>(
                m_generator.getBlock(wx, surfY, wz, surfY, biome));

            // Trees: grass only (shouldPlaceTree also enforces this)
            if (surface == BlockId::Grass && m_generator.shouldPlaceTree(wx, wz)) {
                const int trunkH = m_generator.getTreeHeight(wx, wz);
                placeOakTree(chunk, cx, cz, wx, surfY, wz, trunkH);
                continue;
            }

            const int lx = wx - cx * CHUNK_SIZE;
            const int lz = wz - cz * CHUNK_SIZE;
            if (lx < 0 || lx >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE)
                continue;

            // Cactus / dead shrub: sand edge deserts only (deep desert stays barren)
            if (surface == BlockId::Sand && biome == BiomeType::Desert
                && !m_generator.isDeepDesert(wx, wz)) {
                const uint32_t h = static_cast<uint32_t>(wx * 198491317) ^
                                   static_cast<uint32_t>(wz * 6542989);
                if ((h & 0x3F) == 0) {          // ~1/64
                    placeCactus(chunk, cx, cz, wx, surfY, wz);
                } else if (((h >> 6) & 0x1F) == 0) { // ~1/32, exclusive of cactus
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::DeadShrub);
                }
                continue;
            }

            // Tall grass / flowers: grass surface only (any grass climate, incl. hills)
            if (surface == BlockId::Grass
                && biome == BiomeType::Grassland
                && surfY > WATER_LEVEL) {
                const uint32_t h = static_cast<uint32_t>(wx * 1000003) ^
                                   static_cast<uint32_t>(wz * 999983);
                if ((h & 0x1F) == 0) {          // ~1/32 tall grass
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::TallGrass);
                } else if ((h & 0x7F) == 1) {   // ~1/128 rose
                    setDecorationBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                       BlockId::Rose);
                }
            }
        }
    }
}

void World::fillChunk(Chunk& chunk, int cx, int cz) {
    for (int bx = 0; bx < CHUNK_SIZE; ++bx) {
        for (int bz = 0; bz < CHUNK_SIZE; ++bz) {
            const int worldX = cx * CHUNK_SIZE + bx;
            const int worldZ = cz * CHUNK_SIZE + bz;

            // Height / biome once per column (not once per Y)
            const int height = m_generator.getHeight(worldX, worldZ);
            const BiomeType biome = m_generator.getBiome(worldX, worldZ);

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
    placeDecorations(chunk, cx, cz);
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
        m_queueCv.notify_one();
    }
}

void World::markNeighborsDirty(int cx, int cz) {
    // New column appeared: only remesh neighbor sections that already have blocks
    // (empty upper air sections have no faces that need fixing).
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
                // Shared face only matters if this section has geometry on either side
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
    struct Candidate { int cx, cz, dist2; };
    std::vector<Candidate> missing;
    missing.reserve((RENDER_DISTANCE * 2 + 1) * (RENDER_DISTANCE * 2 + 1));

    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; ++dx) {
        for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; ++dz) {
            int cx = centerCX + dx;
            int cz = centerCZ + dz;
            if (isChunkLoaded(cx, cz)) continue;
            missing.push_back({cx, cz, dx * dx + dz * dz});
        }
    }

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
            m_queueCv.notify_one();
    }
}

void World::integrateGeneratedChunks() {
    std::deque<std::unique_ptr<Chunk>> ready;
    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        ready.swap(m_generatedChunks);
    }

    if (ready.empty()) return;

    std::vector<std::pair<int, int>> inserted;
    inserted.reserve(ready.size());

    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        for (auto& chunk : ready) {
            const int cx = chunk->getCX();
            const int cz = chunk->getCZ();
            uint64_t key = chunkKey(cx, cz);
            if (m_chunks.count(key)) continue;
            m_chunks[key] = std::move(chunk);
            inserted.emplace_back(cx, cz);
        }
    }

    for (auto [cx, cz] : inserted) {
        markMeshDirty(cx, cz);
        markNeighborsDirty(cx, cz);
    }
}

void World::processMeshUploads() {
    std::deque<ChunkCoord> uploads;
    {
        std::lock_guard<std::mutex> qlock(m_queueMutex);
        int n = 0;
        while (!m_uploadQueue.empty() && n < MAX_MESH_UPLOADS_PER_FRAME) {
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
            if (std::abs(dx) > UNLOAD_DISTANCE || std::abs(dz) > UNLOAD_DISTANCE) {
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
    dropFar(m_genQueue, &m_genQueued, RENDER_DISTANCE);
    dropFar(m_meshQueue, &m_meshQueued, UNLOAD_DISTANCE);
    dropFar(m_uploadQueue, nullptr, UNLOAD_DISTANCE);
}

void World::workerLoop() {
    while (true) {
        ChunkCoord genTask{};
        bool hasGen = false;
        ChunkCoord meshTask{};
        bool hasMesh = false;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [&] {
                return !m_running || !m_genQueue.empty() || !m_meshQueue.empty();
            });
            if (!m_running && m_genQueue.empty() && m_meshQueue.empty())
                return;

            // Prefer generation so the world fills in quickly
            if (!m_genQueue.empty()) {
                genTask = m_genQueue.front();
                m_genQueue.pop_front();
                m_genQueued.erase(chunkKey(genTask.cx, genTask.cz));
                hasGen = true;
            } else if (!m_meshQueue.empty()) {
                meshTask = m_meshQueue.front();
                m_meshQueue.pop_front();
                m_meshQueued.erase(chunkKey(meshTask.cx, meshTask.cz));
                hasMesh = true;
            }
        }

        if (hasGen) {
            auto chunk = std::make_unique<Chunk>(genTask.cx, genTask.cz);
            fillChunk(*chunk, genTask.cx, genTask.cz);
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_generatedChunks.push_back(std::move(chunk));
            }
            continue;
        }

        if (hasMesh) {
            Chunk* chunkPtr = nullptr;
            {
                std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
                auto it = m_chunks.find(chunkKey(meshTask.cx, meshTask.cz));
                if (it == m_chunks.end()) continue;
                chunkPtr = it->second.get();
                // buildDirtyMeshes only reads blocks + neighbors via getBlock
                chunkPtr->buildDirtyMeshes(*this);
            }
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_uploadQueue.push_back(meshTask);
            }
        }
    }
}

void World::Update(const glm::vec3& cameraPos) {
    int centerCX = worldToChunk(static_cast<int>(std::floor(cameraPos.x)));
    int centerCZ = worldToChunk(static_cast<int>(std::floor(cameraPos.z)));

    unloadDistantChunks(centerCX, centerCZ);
    enqueueMissingChunks(centerCX, centerCZ);
    integrateGeneratedChunks();
    processMeshUploads();
}

void World::Render(RenderMaster& master, const Camera& camera) {
    Frustum frustum;
    frustum.update(camera.GetProjectionViewMatrix());

    const float renderDist = static_cast<float>((RENDER_DISTANCE + 1) * CHUNK_SIZE);
    const float renderDistSq = renderDist * renderDist;
    const float floraDistSq = FLORA_LOD_DISTANCE * FLORA_LOD_DISTANCE;
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
    master.FinishChunkRender(camera);
}
