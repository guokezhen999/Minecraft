//
// LightEngine.cpp – sky columns + 6-way flood, local box relight
//

#include "LightEngine.h"
#include "World.h"
#include "Chunk/Chunk.h"
#include "Chunk/ChunkSection.h"
#include "Block/ChunkBlock.h"
#include "Block/BlockData.h"
#include "Block/BlockDataBase.h"
#include "Block/BlockId.h"
#include "WorldConstants.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

constexpr int kDX[6] = {1, -1, 0, 0, 0, 0};
constexpr int kDY[6] = {0, 0, 1, -1, 0, 0};
constexpr int kDZ[6] = {0, 0, 0, 0, 1, -1};

uint8_t gOpacity[256];
uint8_t gEmission[256];
bool gLutReady = false;

void ensureLut() {
    if (gLutReady)
        return;
    std::memset(gOpacity, 15, sizeof(gOpacity));
    std::memset(gEmission, 0, sizeof(gEmission));
    gOpacity[static_cast<int>(BlockId::Air)] = 0;
    for (int i = 0; i < static_cast<int>(BlockId::NUM_TYPES); ++i) {
        const auto& d = BlockDatabase::Get().GetData(static_cast<BlockId>(i)).GetBlockData();
        gOpacity[i] = d.lightOpacity;
        gEmission[i] = d.lightEmission;
    }
    gLutReady = true;
}

int packLocal(int x, int y, int z) {
    return x | (z << 4) | (y << 8);
}

// Sky light does not lose intensity falling through air (open pits stay bright).
int lightDecay(bool sky, int dy, int neighborOpacity) {
    if (sky && dy < 0 && neighborOpacity == 0)
        return 0;
    return std::max(1, neighborOpacity);
}

void addRemesh(std::vector<std::pair<int, int>>& remeshCols, int cx, int cz) {
    for (const auto& c : remeshCols) {
        if (c.first == cx && c.second == cz)
            return;
    }
    remeshCols.emplace_back(cx, cz);
}

struct LightWorld {
    World& world;
    Chunk* cached = nullptr;
    int ccx = 0x7fffffff;
    int ccz = 0x7fffffff;

    explicit LightWorld(World& w) : world(w) {}

    Chunk* chunkAt(int wx, int wz) {
        const int cx = World::worldToChunk(wx);
        const int cz = World::worldToChunk(wz);
        if (cx != ccx || cz != ccz) {
            ccx = cx;
            ccz = cz;
            cached = world.getChunkLocked(cx, cz);
        }
        return cached;
    }

    bool loaded(int wx, int wz) { return chunkAt(wx, wz) != nullptr; }

    ChunkBlock block(int x, int y, int z) {
        if (y < 0 || y >= WORLD_HEIGHT)
            return ChunkBlock(BlockId::Air);
        Chunk* c = chunkAt(x, z);
        if (!c)
            return ChunkBlock(BlockId::Air);
        return c->getBlock(x - ccx * CHUNK_SIZE, y, z - ccz * CHUNK_SIZE);
    }

    uint8_t sky(int x, int y, int z) {
        if (y >= WORLD_HEIGHT)
            return static_cast<uint8_t>(LIGHT_LEVEL_MAX);
        if (y < 0)
            return 0;
        Chunk* c = chunkAt(x, z);
        if (!c)
            return 0;
        return c->getSkyLight(x - ccx * CHUNK_SIZE, y, z - ccz * CHUNK_SIZE);
    }

    uint8_t blk(int x, int y, int z) {
        if (y < 0 || y >= WORLD_HEIGHT)
            return 0;
        Chunk* c = chunkAt(x, z);
        if (!c)
            return 0;
        return c->getBlockLight(x - ccx * CHUNK_SIZE, y, z - ccz * CHUNK_SIZE);
    }

    void setSky(int x, int y, int z, uint8_t v) {
        if (y < 0 || y >= WORLD_HEIGHT)
            return;
        Chunk* c = chunkAt(x, z);
        if (!c)
            return;
        c->setSkyLight(x - ccx * CHUNK_SIZE, y, z - ccz * CHUNK_SIZE, v);
    }

    void setBlk(int x, int y, int z, uint8_t v) {
        if (y < 0 || y >= WORLD_HEIGHT)
            return;
        Chunk* c = chunkAt(x, z);
        if (!c)
            return;
        c->setBlockLight(x - ccx * CHUNK_SIZE, y, z - ccz * CHUNK_SIZE, v);
    }

    uint8_t opacity(int x, int y, int z) {
        return gOpacity[block(x, y, z).id];
    }
};

struct SectionDirty {
    std::unordered_map<uint64_t, uint8_t> masks;

    static uint64_t key(int cx, int cz) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
               static_cast<uint32_t>(cz);
    }

    void add(int cx, int cz, int sy) {
        if (sy < 0 || sy >= CHUNK_SECTIONS)
            return;
        masks[key(cx, cz)] |= static_cast<uint8_t>(1u << sy);
    }

    void halo(int wx, int wy, int wz) {
        const int sy0 = std::max(0, (wy - 1) / CHUNK_SIZE);
        const int sy1 = std::min(CHUNK_SECTIONS - 1, (wy + 1) / CHUNK_SIZE);
        const int cx0 = World::worldToChunk(wx - 1);
        const int cx1 = World::worldToChunk(wx + 1);
        const int cz0 = World::worldToChunk(wz - 1);
        const int cz1 = World::worldToChunk(wz + 1);
        for (int cx = cx0; cx <= cx1; ++cx) {
            for (int cz = cz0; cz <= cz1; ++cz) {
                for (int sy = sy0; sy <= sy1; ++sy)
                    add(cx, cz, sy);
            }
        }
    }

    void apply(World& world, std::vector<std::pair<int, int>>& remeshCols) {
        for (auto& [k, mask] : masks) {
            const int cx = static_cast<int>(static_cast<uint32_t>(k >> 32));
            const int cz = static_cast<int>(static_cast<uint32_t>(k));
            Chunk* c = world.getChunkLocked(cx, cz);
            if (!c)
                continue;
            for (int sy = 0; sy < CHUNK_SECTIONS; ++sy) {
                if (mask & static_cast<uint8_t>(1u << sy))
                    c->markSectionDirty(sy);
            }
            addRemesh(remeshCols, cx, cz);
        }
    }
};

void markBoxSections(World& world, int x0, int y0, int z0, int x1, int y1, int z1,
                     std::vector<std::pair<int, int>>& remeshCols) {
    const int sy0 = std::max(0, y0 / CHUNK_SIZE);
    const int sy1 = std::min(CHUNK_SECTIONS - 1, y1 / CHUNK_SIZE);
    const int cx0 = World::worldToChunk(x0);
    const int cx1 = World::worldToChunk(x1);
    const int cz0 = World::worldToChunk(z0);
    const int cz1 = World::worldToChunk(z1);
    for (int cx = cx0; cx <= cx1; ++cx) {
        for (int cz = cz0; cz <= cz1; ++cz) {
            Chunk* c = world.getChunkLocked(cx, cz);
            if (!c)
                continue;
            for (int sy = sy0; sy <= sy1; ++sy)
                c->markSectionDirty(sy);
            addRemesh(remeshCols, cx, cz);
        }
    }
}

void propagateLocalFromQueue(Chunk& chunk, std::vector<int>& q, bool sky) {
    ChunkSection* secs[CHUNK_SECTIONS];
    for (int i = 0; i < CHUNK_SECTIONS; ++i)
        secs[i] = &chunk.getSection(i);

    auto getL = [&](int x, int y, int z) -> uint8_t {
        const int ly = y & 15;
        return sky ? secs[y >> 4]->skyLightRaw(x, ly, z)
                   : secs[y >> 4]->blockLightRaw(x, ly, z);
    };
    auto setL = [&](int x, int y, int z, uint8_t v) {
        const int ly = y & 15;
        if (sky)
            secs[y >> 4]->setSkyLightRaw(x, ly, z, v);
        else
            secs[y >> 4]->setBlockLightRaw(x, ly, z, v);
    };

    size_t head = 0;
    while (head < q.size()) {
        const int p = q[head++];
        const int x = p & 15;
        const int z = (p >> 4) & 15;
        const int y = (p >> 8) & 255;
        const int cur = getL(x, y, z);

        for (int i = 0; i < 6; ++i) {
            const int nx = x + kDX[i];
            const int ny = y + kDY[i];
            const int nz = z + kDZ[i];
            if (nx < 0 || nx >= CHUNK_SIZE || nz < 0 || nz >= CHUNK_SIZE)
                continue;
            if (ny < 0 || ny >= WORLD_HEIGHT)
                continue;

            const int op = static_cast<int>(gOpacity[secs[ny >> 4]->getBlockRaw(nx, ny & 15, nz).id]);
            const int next = cur - lightDecay(sky, kDY[i], op);
            if (next <= 0)
                continue;
            if (next <= getL(nx, ny, nz))
                continue;
            setL(nx, ny, nz, static_cast<uint8_t>(next));
            if (next > 1)
                q.push_back(packLocal(nx, ny, nz));
        }
    }
}

void computeColumnFast(Chunk& chunk) {
    ensureLut();
    ChunkSection* secs[CHUNK_SECTIONS];
    for (int i = 0; i < CHUNK_SECTIONS; ++i)
        secs[i] = &chunk.getSection(i);

    std::vector<int> skyQ;
    skyQ.reserve(1024);

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            bool direct = true;
            for (int y = WORLD_HEIGHT - 1; y >= 0; --y) {
                ChunkSection* s = secs[y >> 4];
                const int ly = y & 15;
                const uint8_t op = gOpacity[s->getBlockRaw(x, ly, z).id];
                uint8_t sky;
                if (direct) {
                    if (op == 0) {
                        sky = static_cast<uint8_t>(LIGHT_LEVEL_MAX);
                    } else {
                        sky = static_cast<uint8_t>(
                            std::max(0, LIGHT_LEVEL_MAX - static_cast<int>(op)));
                        direct = false;
                    }
                } else {
                    sky = 0;
                }
                s->setSkyLightRaw(x, ly, z, sky);
            }
        }
    }

    for (int y = 0; y < WORLD_HEIGHT; ++y) {
        ChunkSection* s = secs[y >> 4];
        const int ly = y & 15;
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                const uint8_t lv = s->skyLightRaw(x, ly, z);
                if (lv <= 1)
                    continue;
                if (lv < LIGHT_LEVEL_MAX) {
                    skyQ.push_back(packLocal(x, y, z));
                    continue;
                }
                bool edge = false;
                for (int i = 0; i < 6 && !edge; ++i) {
                    const int nx = x + kDX[i];
                    const int ny = y + kDY[i];
                    const int nz = z + kDZ[i];
                    if (nx < 0 || nx >= CHUNK_SIZE || nz < 0 || nz >= CHUNK_SIZE)
                        continue;
                    if (ny < 0 || ny >= WORLD_HEIGHT)
                        continue;
                    if (secs[ny >> 4]->skyLightRaw(nx, ny & 15, nz) < LIGHT_LEVEL_MAX)
                        edge = true;
                }
                if (edge)
                    skyQ.push_back(packLocal(x, y, z));
            }
        }
    }
    propagateLocalFromQueue(chunk, skyQ, true);

    std::vector<int> blockQ;
    blockQ.reserve(64);
    for (int y = 0; y < WORLD_HEIGHT; ++y) {
        ChunkSection* s = secs[y >> 4];
        const int ly = y & 15;
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                const uint8_t em = gEmission[s->getBlockRaw(x, ly, z).id];
                s->setBlockLightRaw(x, ly, z, em);
                if (em > 1)
                    blockQ.push_back(packLocal(x, y, z));
            }
        }
    }
    propagateLocalFromQueue(chunk, blockQ, false);
}

void propagateWorldBounded(LightWorld& lw, std::vector<glm::ivec3>& q, bool sky,
                           int x0, int y0, int z0, int x1, int y1, int z1,
                           SectionDirty* dirty) {
    auto getL = [&](int x, int y, int z) {
        return sky ? lw.sky(x, y, z) : lw.blk(x, y, z);
    };
    auto setL = [&](int x, int y, int z, uint8_t v) {
        if (sky)
            lw.setSky(x, y, z, v);
        else
            lw.setBlk(x, y, z, v);
    };

    const bool bounded = (x1 >= x0);
    size_t head = 0;
    while (head < q.size()) {
        const glm::ivec3 p = q[head++];
        const int cur = getL(p.x, p.y, p.z);

        for (int i = 0; i < 6; ++i) {
            const int nx = p.x + kDX[i];
            const int ny = p.y + kDY[i];
            const int nz = p.z + kDZ[i];
            if (ny < 0 || ny >= WORLD_HEIGHT)
                continue;
            if (bounded && (nx < x0 || nx > x1 || ny < y0 || ny > y1 || nz < z0 || nz > z1))
                continue;
            if (!lw.loaded(nx, nz))
                continue;

            const int next = cur - lightDecay(sky, kDY[i], static_cast<int>(lw.opacity(nx, ny, nz)));
            if (next <= 0)
                continue;
            if (next <= getL(nx, ny, nz))
                continue;
            setL(nx, ny, nz, static_cast<uint8_t>(next));
            if (dirty)
                dirty->halo(nx, ny, nz);
            if (next > 1)
                q.push_back({nx, ny, nz});
        }
    }
}

void seedGradient(LightWorld& lw, int x, int y, int z, int nx, int ny, int nz,
                  std::vector<glm::ivec3>& skyQ, std::vector<glm::ivec3>& blockQ) {
    if (!lw.loaded(x, z) || !lw.loaded(nx, nz))
        return;
    if (ny < 0 || ny >= WORLD_HEIGHT || y < 0 || y >= WORLD_HEIGHT)
        return;

    const int s0 = lw.sky(x, y, z);
    const int s1 = lw.sky(nx, ny, nz);
    if (s0 > 1 && s0 - lightDecay(true, ny - y, static_cast<int>(lw.opacity(nx, ny, nz))) > s1)
        skyQ.push_back({x, y, z});
    if (s1 > 1 && s1 - lightDecay(true, y - ny, static_cast<int>(lw.opacity(x, y, z))) > s0)
        skyQ.push_back({nx, ny, nz});

    const int b0 = lw.blk(x, y, z);
    const int b1 = lw.blk(nx, ny, nz);
    if (b0 > 1 && b0 - std::max(1, static_cast<int>(lw.opacity(nx, ny, nz))) > b1)
        blockQ.push_back({x, y, z});
    if (b1 > 1 && b1 - std::max(1, static_cast<int>(lw.opacity(x, y, z))) > b0)
        blockQ.push_back({nx, ny, nz});
}

void relightBox(World& world, int ox, int oy, int oz,
                std::vector<std::pair<int, int>>& remeshCols) {
    ensureLut();
    const int r = LIGHT_EDIT_RADIUS;
    const int x0 = ox - r;
    const int x1 = ox + r;
    const int z0 = oz - r;
    const int z1 = oz + r;
    const int y0 = std::max(0, oy - r);
    const int y1 = std::min(WORLD_HEIGHT - 1, oy + r);

    LightWorld lw(world);

    for (int z = z0; z <= z1; ++z) {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if (!lw.loaded(x, z))
                    continue;
                lw.setSky(x, y, z, 0);
                lw.setBlk(x, y, z, 0);
            }
        }
    }

    std::vector<glm::ivec3> skyQ;
    std::vector<glm::ivec3> blockQ;
    skyQ.reserve(4096);
    blockQ.reserve(256);

    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            if (!lw.loaded(x, z))
                continue;
            bool direct = true;
            for (int y = WORLD_HEIGHT - 1; y >= y0; --y) {
                const uint8_t op = lw.opacity(x, y, z);
                uint8_t sky;
                if (direct) {
                    if (op == 0) {
                        sky = static_cast<uint8_t>(LIGHT_LEVEL_MAX);
                    } else {
                        sky = static_cast<uint8_t>(
                            std::max(0, LIGHT_LEVEL_MAX - static_cast<int>(op)));
                        direct = false;
                    }
                } else {
                    sky = 0;
                }
                if (y <= y1) {
                    lw.setSky(x, y, z, sky);
                    if (sky > 1)
                        skyQ.push_back({x, y, z});
                }
            }
        }
    }

    for (int z = z0; z <= z1; ++z) {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if (!lw.loaded(x, z))
                    continue;
                const uint8_t em = gEmission[lw.block(x, y, z).id];
                lw.setBlk(x, y, z, em);
                if (em > 1)
                    blockQ.push_back({x, y, z});
            }
        }
    }

    auto seedOutside = [&](int x, int y, int z) {
        if (y < 0 || y >= WORLD_HEIGHT || !lw.loaded(x, z))
            return;
        if (lw.sky(x, y, z) > 1)
            skyQ.push_back({x, y, z});
        if (lw.blk(x, y, z) > 1)
            blockQ.push_back({x, y, z});
    };

    for (int y = y0; y <= y1; ++y) {
        for (int z = z0; z <= z1; ++z) {
            seedOutside(x0 - 1, y, z);
            seedOutside(x1 + 1, y, z);
        }
        for (int x = x0; x <= x1; ++x) {
            seedOutside(x, y, z0 - 1);
            seedOutside(x, y, z1 + 1);
        }
    }
    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            seedOutside(x, y0 - 1, z);
            seedOutside(x, y1 + 1, z);
        }
    }

    propagateWorldBounded(lw, skyQ, true, x0, y0, z0, x1, y1, z1, nullptr);
    propagateWorldBounded(lw, blockQ, false, x0, y0, z0, x1, y1, z1, nullptr);

    markBoxSections(world, x0 - 1, std::max(0, y0 - 1), z0 - 1,
                    x1 + 1, std::min(WORLD_HEIGHT - 1, y1 + 1), z1 + 1,
                    remeshCols);
}

} // namespace

uint8_t LightEngine::opacityOf(ChunkBlock block) {
    ensureLut();
    return gOpacity[block.id];
}

uint8_t LightEngine::emissionOf(ChunkBlock block) {
    ensureLut();
    return gEmission[block.id];
}

void LightEngine::computeColumn(Chunk& chunk) {
    computeColumnFast(chunk);
}

void LightEngine::propagateFromNeighbors(World& world, int cx, int cz,
                                         std::vector<std::pair<int, int>>& remeshCols) {
    ensureLut();
    if (!world.getChunkLocked(cx, cz))
        return;

    LightWorld lw(world);
    std::vector<glm::ivec3> skyQ;
    std::vector<glm::ivec3> blockQ;
    skyQ.reserve(256);
    blockQ.reserve(64);

    const int x0 = cx * CHUNK_SIZE;
    const int z0 = cz * CHUNK_SIZE;
    for (int y = 0; y < WORLD_HEIGHT; ++y) {
        for (int i = 0; i < CHUNK_SIZE; ++i) {
            seedGradient(lw, x0, y, z0 + i, x0 - 1, y, z0 + i, skyQ, blockQ);
            seedGradient(lw, x0 + CHUNK_SIZE - 1, y, z0 + i,
                         x0 + CHUNK_SIZE, y, z0 + i, skyQ, blockQ);
            seedGradient(lw, x0 + i, y, z0, x0 + i, y, z0 - 1, skyQ, blockQ);
            seedGradient(lw, x0 + i, y, z0 + CHUNK_SIZE - 1,
                         x0 + i, y, z0 + CHUNK_SIZE, skyQ, blockQ);
        }
    }

    if (skyQ.empty() && blockQ.empty())
        return;

    SectionDirty dirty;
    propagateWorldBounded(lw, skyQ, true, 0, 0, 0, -1, -1, -1, &dirty);
    propagateWorldBounded(lw, blockQ, false, 0, 0, 0, -1, -1, -1, &dirty);
    dirty.apply(world, remeshCols);
}

void LightEngine::relightColumns(World& world,
                                 const std::vector<std::pair<int, int>>& cols,
                                 std::vector<std::pair<int, int>>& remeshCols) {
    std::unordered_set<uint64_t> seen;
    std::vector<std::pair<int, int>> unique;
    unique.reserve(cols.size());
    for (auto [cx, cz] : cols) {
        const uint64_t key =
            (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
            static_cast<uint32_t>(cz);
        if (!seen.insert(key).second)
            continue;
        if (!world.getChunkLocked(cx, cz))
            continue;
        unique.emplace_back(cx, cz);
    }

    for (auto [cx, cz] : unique)
        computeColumnFast(*world.getChunkLocked(cx, cz));
    for (auto [cx, cz] : unique)
        propagateFromNeighbors(world, cx, cz, remeshCols);

    for (auto [cx, cz] : unique) {
        Chunk* c = world.getChunkLocked(cx, cz);
        if (!c)
            continue;
        c->markNonEmptySectionsDirty();
        addRemesh(remeshCols, cx, cz);
    }
}

void LightEngine::updateAfterEdit(World& world, int x, int y, int z,
                                  std::vector<std::pair<int, int>>& remeshCols) {
    relightBox(world, x, y, z, remeshCols);
}
