//
// World save I/O: settings.cfg, world.dat, dirty column files, save listing
//

#ifndef MINECRAFT_WORLDSAVE_H
#define MINECRAFT_WORLDSAVE_H

#include "../Config.h"
#include "../World/Block/BlockId.h"
#include "../UI/Hotbar.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class Chunk;

namespace WorldSave {

constexpr int WORLD_DAT_VERSION = 1;
constexpr uint32_t COLUMN_MAGIC = 0x4843434Du; // 'MCCH' little-endian
constexpr uint32_t COLUMN_VERSION = 1;

struct WorldHeader {
    int version = WORLD_DAT_VERSION;
    std::string name = "New World";
    int32_t seed = 0;
    float playerX = 0.5f;
    float playerY = 64.0f;
    float playerZ = 0.5f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    bool flying = false;
    std::array<int, Hotbar::SLOT_COUNT> hotbar{
        static_cast<int>(BlockId::Stone),
        static_cast<int>(BlockId::Dirt),
        static_cast<int>(BlockId::Grass),
        static_cast<int>(BlockId::Sand),
        static_cast<int>(BlockId::OakBark),
        static_cast<int>(BlockId::OakLeaf),
        static_cast<int>(BlockId::Cactus),
        static_cast<int>(BlockId::Water),
        static_cast<int>(BlockId::Torch),
    };
    int selected = 0;
    uint64_t gameTime = 6000; // noon; same default as World
    int64_t created = 0;
    int64_t lastPlayed = 0;
};

struct WorldInfo {
    std::string folder;
    WorldHeader header;
    bool corrupt = false;
};

std::string worldsRoot();
std::string worldDir(const std::string& folder);
std::string worldDatPath(const std::string& folder);
std::string columnPath(const std::string& worldDirectory, int cx, int cz);

void ensureSavesDirs();

std::string slugify(const std::string& name);
std::string uniqueFolder(const std::string& slug);

std::vector<WorldInfo> listWorlds();

bool readWorldDat(const std::string& folder, WorldHeader& out);
bool writeWorldDat(const std::string& folder, const WorldHeader& data);

bool loadColumn(const std::string& worldDirectory, int cx, int cz, Chunk& chunk);
bool saveColumn(const std::string& worldDirectory, int cx, int cz, const Chunk& chunk);

bool deleteWorld(const std::string& folder);
bool worldExists(const std::string& folder);

bool loadSettings(Config& config);
bool saveSettings(const Config& config);

int32_t randomSeed();
bool parseSeed(const std::string& text, int32_t& out);

std::string formatTime(int64_t unixSeconds);

} // namespace WorldSave

#endif // MINECRAFT_WORLDSAVE_H
