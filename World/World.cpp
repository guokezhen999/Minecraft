//
// World.cpp – async chunk gen/mesh, frustum cull, flora LOD
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
    if (worldY < 0 || worldY >= CHUNK_SIZE) {
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

bool World::isCollidable(int worldX, int worldY, int worldZ) const {
    if (worldY < 0)
        return true;
    if (worldY >= CHUNK_SIZE)
        return false;

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);

    std::shared_lock<std::shared_mutex> lock(m_chunkMutex);
    if (m_chunks.find(chunkKey(cx, cz)) == m_chunks.end())
        return true; // Unloaded: treat as solid so the player does not fall through

    return getBlockLocked(worldX, worldY, worldZ).GetData().isCollidable;
}

bool World::setBlock(int worldX, int worldY, int worldZ, ChunkBlock block) {
    if (worldY < 0 || worldY >= CHUNK_SIZE)
        return false;

    const int cx = worldToChunk(worldX);
    const int cz = worldToChunk(worldZ);
    const int lx = worldX - cx * CHUNK_SIZE;
    const int lz = worldZ - cz * CHUNK_SIZE;

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
        if (lx == 0)
            toRemesh.emplace_back(cx - 1, cz);
        if (lx == CHUNK_SIZE - 1)
            toRemesh.emplace_back(cx + 1, cz);
        if (lz == 0)
            toRemesh.emplace_back(cx, cz - 1);
        if (lz == CHUNK_SIZE - 1)
            toRemesh.emplace_back(cx, cz + 1);

        // Sync remesh so dig/place is visible immediately
        for (auto [rcx, rcz] : toRemesh) {
            auto rit = m_chunks.find(chunkKey(rcx, rcz));
            if (rit == m_chunks.end())
                continue;
            rit->second->buildMesh(*this);
        }
    }

    // Drop any pending async remesh for these chunks (we already rebuilt)
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
            rit->second->bufferMeshes();
    }
    return true;
}

void World::setWorldBlock(Chunk& chunk, int cx, int cz,
                           int wx, int wy, int wz, BlockId id) {
    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    if (lx < 0 || lx >= CHUNK_SIZE) return;
    if (lz < 0 || lz >= CHUNK_SIZE) return;
    if (wy < 0 || wy >= CHUNK_SIZE) return;
    chunk.setBlock(lx, wy, lz, ChunkBlock(id));
}

void World::placeOakTree(Chunk& chunk, int cx, int cz,
                          int wx, int surfY, int wz, int trunkHeight) {
    for (int ty = surfY + 1; ty <= surfY + trunkHeight; ++ty) {
        setWorldBlock(chunk, cx, cz, wx, ty, wz, BlockId::OakBark);
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
                setWorldBlock(chunk, cx, cz,
                              wx + dx, ly, wz + dz, BlockId::OakLeaf);
            }
        }
    }
}

void World::placeCactus(Chunk& chunk, int cx, int cz, int wx, int surfY, int wz) {
    int h = ((wx * 1234567) ^ (wz * 7654321)) & 0x3;
    h = std::max(1, h);
    for (int ty = surfY + 1; ty <= surfY + h; ++ty) {
        setWorldBlock(chunk, cx, cz, wx, ty, wz, BlockId::Cactus);
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
            BiomeType biome = m_generator.getBiome(wx, wz);
            int surfY = m_generator.getHeight(wx, wz);

            if (m_generator.shouldPlaceTree(wx, wz)) {
                int trunkH = m_generator.getTreeHeight(wx, wz);
                placeOakTree(chunk, cx, cz, wx, surfY, wz, trunkH);

            } else if (biome == BiomeType::Desert) {
                int lx = wx - cx * CHUNK_SIZE;
                int lz = wz - cz * CHUNK_SIZE;
                if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE) {
                    uint32_t h = static_cast<uint32_t>(wx * 198491317) ^
                                 static_cast<uint32_t>(wz * 6542989);
                    if ((h & 0x1F) == 0) {
                        placeCactus(chunk, cx, cz, wx, surfY, wz);
                    }

                    if (((h >> 5) & 0xF) == 0) {
                        if (surfY + 1 < CHUNK_SIZE)
                            setWorldBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                          BlockId::DeadShrub);
                    }
                }

            } else if (biome == BiomeType::Grassland) {
                int lx = wx - cx * CHUNK_SIZE;
                int lz = wz - cz * CHUNK_SIZE;
                if (lx >= 0 && lx < CHUNK_SIZE && lz >= 0 && lz < CHUNK_SIZE
                    && surfY > WATER_LEVEL + 1) {
                    uint32_t h = static_cast<uint32_t>(wx * 1000003) ^
                                 static_cast<uint32_t>(wz * 999983);
                    if ((h & 0x7) == 0 && surfY + 1 < CHUNK_SIZE) {
                        setWorldBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                      BlockId::TallGrass);
                    } else if ((h & 0x1F) == 1 && surfY + 1 < CHUNK_SIZE) {
                        setWorldBlock(chunk, cx, cz, wx, surfY + 1, wz,
                                      BlockId::Rose);
                    }
                }
            }
        }
    }
}

void World::fillChunk(Chunk& chunk, int cx, int cz) {
    for (int bx = 0; bx < CHUNK_SIZE; ++bx) {
        for (int bz = 0; bz < CHUNK_SIZE; ++bz) {
            int worldX = cx * CHUNK_SIZE + bx;
            int worldZ = cz * CHUNK_SIZE + bz;

            for (int by = 0; by < CHUNK_SIZE; ++by) {
                int blockType = m_generator.getBlock(worldX, by, worldZ);
                if (blockType != static_cast<int>(BlockId::Air)) {
                    chunk.setBlock(bx, by, bz,
                                   ChunkBlock(static_cast<BlockId>(blockType)));
                }
            }
        }
    }
    placeDecorations(chunk, cx, cz);
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
            // May race with integrate; worker/gen still fine if already loaded
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
            const sf::Vector3i& loc = chunk->getLocation();
            uint64_t key = chunkKey(loc.x, loc.z);
            if (m_chunks.count(key)) continue;
            m_chunks[key] = std::move(chunk);
            inserted.emplace_back(loc.x, loc.z);
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
            it->second->bufferMeshes();
        }
    }
}

void World::unloadDistantChunks(int centerCX, int centerCZ) {
    {
        std::unique_lock<std::shared_mutex> lock(m_chunkMutex);
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
            const sf::Vector3i& loc = it->second->getLocation();
            int dx = loc.x - centerCX;
            int dz = loc.z - centerCZ;
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
            auto chunk = std::make_unique<Chunk>(sf::Vector3i(genTask.cx, 0, genTask.cz));
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
                // buildMesh only reads blocks + neighbors via getBlock (shared)
                chunkPtr->buildMesh(*this);
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

        const sf::Vector3i& loc = chunk->getLocation();
        if (!frustum.intersectsChunkColumn(loc.x, loc.z, CHUNK_SIZE))
            continue;

        const float chunkCX = (loc.x + 0.5f) * CHUNK_SIZE;
        const float chunkCZ = (loc.z + 0.5f) * CHUNK_SIZE;
        const float dx = chunkCX - camPos.x;
        const float dz = chunkCZ - camPos.z;
        const float distSq = dx * dx + dz * dz;

        if (distSq > renderDistSq) continue;

        const bool drawFlora = distSq <= floraDistSq;
        master.DrawChunk(chunk->getMeshes(), distSq, drawFlora);
    }

    lock.unlock();
    master.FinishChunkRender(camera);
}
