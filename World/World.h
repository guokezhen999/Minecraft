//
// World.h – chunk streaming with async gen/mesh, frustum cull, LOD
// Vertical columns of ChunkSections (WORLD_HEIGHT)
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
#include <string>
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
    World(int seed, std::string savePath, int renderDistance);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Update(const glm::vec3& cameraPos);
    void Render(RenderMaster& master, const Camera& camera, bool underwater,
                const Atmosphere& atmosphere);

    // True if eyes are below the water surface in the current cell
    bool isCameraUnderwater(const glm::vec3& eyePos) const;

    // Thread-safe block query
    ChunkBlock getBlock(int worldX, int worldY, int worldZ) const;

    // Caller must already hold a shared/unique lock on the chunk map
    ChunkBlock getBlockLocked(int worldX, int worldY, int worldZ) const;

    uint8_t getSkyLight(int worldX, int worldY, int worldZ) const;
    uint8_t getBlockLight(int worldX, int worldY, int worldZ) const;
    uint8_t getSkyLightLocked(int worldX, int worldY, int worldZ) const;
    uint8_t getBlockLightLocked(int worldX, int worldY, int worldZ) const;
    void getLightsLocked(int worldX, int worldY, int worldZ,
                         uint8_t& sky, uint8_t& block) const;
    void setSkyLightLocked(int worldX, int worldY, int worldZ, uint8_t value);
    void setBlockLightLocked(int worldX, int worldY, int worldZ, uint8_t value);

    Chunk* getChunkLocked(int cx, int cz);
    const Chunk* getChunkLocked(int cx, int cz) const;

    // Edit world block and remesh affected sections (main / GL thread).
    // Returns false if Y out of range or the chunk is not loaded.
    bool setBlock(int worldX, int worldY, int worldZ, ChunkBlock block);

    // Collision: unloaded chunks and y < 0 count as solid (no void fall-through).
    bool isCollidable(int worldX, int worldY, int worldZ) const;

    bool isChunkLoaded(int cx, int cz) const;

    BiomeType getBiome(int worldX, int worldZ) const;

    // Terrain surface Y at column (same as generator; ignores player edits)
    int getSurfaceHeight(int worldX, int worldZ) const;

    void advanceTime(int ticks);
    uint64_t getWorldTick() const { return m_worldTick; }
    void setWorldTick(uint64_t tick) { m_worldTick = tick; }
    Atmosphere getAtmosphere() const;

    void flushDirtyColumns();
    void setRenderDistance(int distance);
    int getRenderDistance() const { return m_renderDistance; }
    int getSeed() const;
    const std::string& savePath() const { return m_savePath; }

    static int worldToChunk(int worldCoord);

private:
    struct ChunkCoord {
        int cx, cz;
    };

    struct BlockPos {
        int x, y, z;
        bool operator==(const BlockPos& o) const {
            return x == o.x && y == o.y && z == o.z;
        }
    };

    struct BlockPosHash {
        size_t operator()(const BlockPos& p) const {
            size_t h = static_cast<size_t>(p.x) * 73856093u;
            h ^= static_cast<size_t>(p.y) * 19349663u;
            h ^= static_cast<size_t>(p.z) * 83492791u;
            return h;
        }
    };

    void genWorkerLoop();
    void meshWorkerLoop();

    void enqueueMissingChunks(int centerCX, int centerCZ);
    void integrateGeneratedChunks(int budget);
    void processMeshUploads(int budget);
    void unloadDistantChunks(int centerCX, int centerCZ);
    bool isRingClaimed(int centerCX, int centerCZ, int radius) const;
    bool isStreaming() const;

    void fillChunk(Chunk& chunk, int cx, int cz);
    bool tryLoadColumn(Chunk& chunk, int cx, int cz);
    int unloadDistance() const { return m_renderDistance + 2; }
    void placeDecorations(Chunk& chunk, int cx, int cz,
                          const TerrainColumn interior[CHUNK_SIZE][CHUNK_SIZE]);
    void placeOakTree(Chunk& chunk, int cx, int cz,
                      int wx, int surfY, int wz, int trunkHeight,
                      BlockId bark, BlockId leaf);
    void placeSpruceTree(Chunk& chunk, int cx, int cz,
                         int wx, int surfY, int wz, int trunkHeight);
    void placeJungleTree(Chunk& chunk, int cx, int cz,
                         int wx, int surfY, int wz, int trunkHeight);
    void placeCactus(Chunk& chunk, int cx, int cz, int wx, int surfY, int wz);
    void setWorldBlock(Chunk& chunk, int cx, int cz,
                       int wx, int wy, int wz, BlockId id);
    // Decorations only: never overwrite water / solid terrain
    void setDecorationBlock(Chunk& chunk, int cx, int cz,
                            int wx, int wy, int wz, BlockId id);

    void markMeshDirty(int cx, int cz);
    void markNeighborsDirty(int cx, int cz);

    void scheduleFluidUpdate(int x, int y, int z);
    void scheduleFluidAround(int x, int y, int z);
    void updateFluids();
    void updateFluidAt(int x, int y, int z,
                       std::vector<std::pair<int, int>>& remeshCols);

    // Requires unique lock on m_chunkMutex. Queues remesh column; optional fluid.
    bool setBlockDeferred(int worldX, int worldY, int worldZ, ChunkBlock block,
                          std::vector<std::pair<int, int>>& remeshCols,
                          bool scheduleFluid);

    void remeshAndUploadColumns(const std::vector<std::pair<int, int>>& cols);

    void noteLightDirty(int cx, int cz);
    void flushLightUpdates(std::vector<std::pair<int, int>>& remeshCols);

    static uint64_t chunkKey(int cx, int cz) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
                static_cast<uint32_t>(cz);
    }

    mutable std::shared_mutex m_chunkMutex;
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> m_chunks;
    TerrainGenerator m_generator;
    std::string m_savePath;
    int m_renderDistance = RENDER_DISTANCE;
    int m_streamRadius = STREAM_START_RADIUS;
    uint64_t m_worldTick = 6000; // noon

    // Fluid update queue (main thread only)
    std::deque<BlockPos> m_fluidQueue;
    std::unordered_set<BlockPos, BlockPosHash> m_fluidQueued;
    std::unordered_set<uint64_t> m_lightDirty;

    // ── Worker queues ───────────────────────────────────────────────────────
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::deque<ChunkCoord> m_genQueue;
    std::unordered_set<uint64_t> m_genQueued;
    std::deque<ChunkCoord> m_meshQueue;
    std::unordered_set<uint64_t> m_meshQueued;

    std::deque<std::unique_ptr<Chunk>> m_generatedChunks;
    std::deque<ChunkCoord> m_uploadQueue;

    std::atomic<bool> m_running{true};
    std::thread m_genWorker;
    std::thread m_meshWorker;
};

#endif //MINECRAFT_WORLD_H
