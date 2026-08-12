//
// World.h – chunk streaming with async gen/mesh, frustum cull, LOD
//

#ifndef MINECRAFT_WORLD_H
#define MINECRAFT_WORLD_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <memory>
#include <cstdint>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <shared_mutex>

#include <glm/glm.hpp>

#include "WorldConstants.h"
#include "TerrainGenerator.h"
#include "Chunk/Chunk.h"
#include "Block/ChunkBlock.h"

class RenderMaster;
class Camera;

class World {
public:
    explicit World(int seed = 0);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Update(const glm::vec3& cameraPos);
    void Render(RenderMaster& master, const Camera& camera);

    // Thread-safe block query
    ChunkBlock getBlock(int worldX, int worldY, int worldZ) const;

    // Caller must already hold a shared/unique lock on the chunk map
    ChunkBlock getBlockLocked(int worldX, int worldY, int worldZ) const;

    bool isChunkLoaded(int cx, int cz) const;

private:
    struct ChunkCoord {
        int cx, cz;
    };

    void workerLoop();

    void enqueueMissingChunks(int centerCX, int centerCZ);
    void integrateGeneratedChunks();
    void processMeshUploads();
    void unloadDistantChunks(int centerCX, int centerCZ);

    void fillChunk(Chunk& chunk, int cx, int cz);
    void placeDecorations(Chunk& chunk, int cx, int cz);
    void placeOakTree(Chunk& chunk, int cx, int cz,
                      int wx, int surfY, int wz, int trunkHeight);
    void placeCactus(Chunk& chunk, int cx, int cz, int wx, int surfY, int wz);
    void setWorldBlock(Chunk& chunk, int cx, int cz,
                       int wx, int wy, int wz, BlockId id);

    void markMeshDirty(int cx, int cz);
    void markNeighborsDirty(int cx, int cz);

    static uint64_t chunkKey(int cx, int cz) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
                static_cast<uint32_t>(cz);
    }

    static int worldToChunk(int worldCoord);

    mutable std::shared_mutex m_chunkMutex;
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> m_chunks;
    TerrainGenerator m_generator;

    // ── Worker queues ───────────────────────────────────────────────────────
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::deque<ChunkCoord> m_genQueue;
    std::unordered_set<uint64_t> m_genQueued;
    std::deque<ChunkCoord> m_meshQueue;
    std::unordered_set<uint64_t> m_meshQueued;

    std::deque<std::unique_ptr<Chunk>> m_generatedChunks;
    std::deque<ChunkCoord> m_uploadQueue;

    std::atomic<bool> m_running{true};
    std::thread m_worker;
};

#endif //MINECRAFT_WORLD_H
