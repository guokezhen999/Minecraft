//
// World save I/O: settings.cfg, world.dat, dirty column files, save listing
//

#include "WorldSave.h"
#include "Chunk/Chunk.h"
#include "WorldConstants.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <system_error>

#include <filesystem>

namespace fs = std::filesystem;

namespace WorldSave {
namespace {

void writeU32(std::ostream& out, uint32_t v) {
    char b[4];
    b[0] = static_cast<char>(v & 0xFFu);
    b[1] = static_cast<char>((v >> 8) & 0xFFu);
    b[2] = static_cast<char>((v >> 16) & 0xFFu);
    b[3] = static_cast<char>((v >> 24) & 0xFFu);
    out.write(b, 4);
}

void writeI32(std::ostream& out, int32_t v) {
    writeU32(out, static_cast<uint32_t>(v));
}

bool readU32(std::istream& in, uint32_t& v) {
    char b[4];
    if (!in.read(b, 4))
        return false;
    v = static_cast<uint32_t>(static_cast<unsigned char>(b[0])) |
        (static_cast<uint32_t>(static_cast<unsigned char>(b[1])) << 8) |
        (static_cast<uint32_t>(static_cast<unsigned char>(b[2])) << 16) |
        (static_cast<uint32_t>(static_cast<unsigned char>(b[3])) << 24);
    return true;
}

bool readI32(std::istream& in, int32_t& v) {
    uint32_t u = 0;
    if (!readU32(in, u))
        return false;
    v = static_cast<int32_t>(u);
    return true;
}

std::string trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
        ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
        --b;
    return s.substr(a, b - a);
}

void packChunk(const Chunk& chunk, uint8_t* ids, uint8_t* metas) {
    int i = 0;
    for (int sy = 0; sy < CHUNK_SECTIONS; ++sy) {
        for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
            const int wy = sy * CHUNK_SIZE + ly;
            for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
                for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
                    const ChunkBlock b = chunk.getBlock(lx, wy, lz);
                    ids[i] = b.id;
                    metas[i] = b.meta;
                    ++i;
                }
            }
        }
    }
}

void unpackChunk(Chunk& chunk, const uint8_t* ids, const uint8_t* metas) {
    int i = 0;
    const int maxId = static_cast<int>(BlockId::NUM_TYPES);
    for (int sy = 0; sy < CHUNK_SECTIONS; ++sy) {
        for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
            const int wy = sy * CHUNK_SIZE + ly;
            for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
                for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
                    ChunkBlock b;
                    const int id = ids[i];
                    b.id = (id > 0 && id < maxId) ? static_cast<Block_t>(id) : 0;
                    b.meta = metas[i];
                    chunk.setBlockRaw(lx, wy, lz, b);
                    ++i;
                }
            }
        }
    }
}

template <typename T>
T clampVal(T v, T lo, T hi) {
    return std::max(lo, std::min(hi, v));
}

} // namespace

std::string worldsRoot() {
    return "saves/worlds";
}

std::string worldDir(const std::string& folder) {
    return (fs::path("saves") / "worlds" / folder).string();
}

std::string worldDatPath(const std::string& folder) {
    return (fs::path(worldDir(folder)) / "world.dat").string();
}

std::string columnPath(const std::string& worldDirectory, int cx, int cz) {
    return (fs::path(worldDirectory) / ("c." + std::to_string(cx) + "." + std::to_string(cz))).string();
}

void ensureSavesDirs() {
    std::error_code ec;
    fs::create_directories("saves/worlds", ec);
}

std::string slugify(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    bool lastUnderscore = false;
    for (unsigned char c : name) {
        if (std::isupper(c))
            c = static_cast<unsigned char>(std::tolower(c));
        if (c == ' ')
            c = '_';
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
            if (c == '_' && lastUnderscore)
                continue;
            out.push_back(static_cast<char>(c));
            lastUnderscore = (c == '_');
        }
    }
    while (!out.empty() && out.front() == '_')
        out.erase(out.begin());
    while (!out.empty() && out.back() == '_')
        out.pop_back();
    if (out.empty())
        out = "world";
    return out;
}

std::string uniqueFolder(const std::string& slug) {
    ensureSavesDirs();
    std::string candidate = slug.empty() ? "world" : slug;
    if (!fs::exists(worldDir(candidate)))
        return candidate;
    for (int n = 2; n < 10000; ++n) {
        const std::string next = candidate + "_" + std::to_string(n);
        if (!fs::exists(worldDir(next)))
            return next;
    }
    return candidate + "_x";
}

bool readWorldDat(const std::string& folder, WorldHeader& out) {
    std::ifstream in(worldDatPath(folder));
    if (!in)
        return false;

    WorldHeader h;
    bool haveVersion = false;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
            continue;
        if (key == "Version") {
            iss >> h.version;
            haveVersion = true;
        } else if (key == "Name") {
            std::string rest;
            std::getline(iss, rest);
            h.name = trim(rest);
            if (h.name.empty())
                h.name = "New World";
        } else if (key == "Seed") {
            iss >> h.seed;
        } else if (key == "Player") {
            iss >> h.playerX >> h.playerY >> h.playerZ;
        } else if (key == "Look") {
            iss >> h.yaw >> h.pitch;
        } else if (key == "Flying") {
            int f = 0;
            iss >> f;
            h.flying = f != 0;
        } else if (key == "Hotbar") {
            for (int i = 0; i < Hotbar::SLOT_COUNT; ++i)
                iss >> h.hotbar[i];
        } else if (key == "Selected") {
            iss >> h.selected;
        } else if (key == "Created") {
            iss >> h.created;
        } else if (key == "LastPlayed") {
            iss >> h.lastPlayed;
        } else if (key == "GameTime") {
            iss >> h.gameTime;
        }
    }

    if (!haveVersion || h.version != WORLD_DAT_VERSION)
        return false;

    h.selected = clampVal(h.selected, 0, Hotbar::SLOT_COUNT - 1);
    const int maxId = static_cast<int>(BlockId::NUM_TYPES) - 1;
    for (int& id : h.hotbar)
        id = clampVal(id, 0, maxId);
    out = h;
    return true;
}

bool writeWorldDat(const std::string& folder, const WorldHeader& data) {
    ensureSavesDirs();
    std::error_code ec;
    fs::create_directories(worldDir(folder), ec);
    if (ec)
        return false;

    const std::string path = worldDatPath(folder);
    const std::string tmp = path + ".tmp";
    std::ofstream out(tmp, std::ios::trunc);
    if (!out)
        return false;

    out << "Version " << WORLD_DAT_VERSION << "\n";
    out << "Name " << data.name << "\n";
    out << "Seed " << data.seed << "\n";
    out << "Player " << data.playerX << " " << data.playerY << " " << data.playerZ << "\n";
    out << "Look " << data.yaw << " " << data.pitch << "\n";
    out << "Flying " << (data.flying ? 1 : 0) << "\n";
    out << "Hotbar";
    for (int i = 0; i < Hotbar::SLOT_COUNT; ++i)
        out << " " << data.hotbar[i];
    out << "\n";
    out << "Selected " << data.selected << "\n";
    out << "Created " << data.created << "\n";
    out << "LastPlayed " << data.lastPlayed << "\n";
    out << "GameTime " << data.gameTime << "\n";
    out.flush();
    if (!out)
        return false;
    out.close();

    fs::rename(tmp, path, ec);
    if (ec) {
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
    }
    return !ec;
}

std::vector<WorldInfo> listWorlds() {
    ensureSavesDirs();
    std::vector<WorldInfo> list;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(worldsRoot(), ec)) {
        if (ec)
            break;
        if (!entry.is_directory())
            continue;
        const std::string folder = entry.path().filename().string();
        if (!fs::exists(worldDatPath(folder)))
            continue;
        WorldInfo info;
        info.folder = folder;
        if (!readWorldDat(folder, info.header))
            info.corrupt = true;
        list.push_back(std::move(info));
    }
    std::sort(list.begin(), list.end(), [](const WorldInfo& a, const WorldInfo& b) {
        if (a.corrupt != b.corrupt)
            return !a.corrupt && b.corrupt;
        return a.header.lastPlayed > b.header.lastPlayed;
    });
    return list;
}

bool loadColumn(const std::string& worldDirectory, int cx, int cz, Chunk& chunk) {
    std::ifstream in(columnPath(worldDirectory, cx, cz), std::ios::binary);
    if (!in)
        return false;

    uint32_t magic = 0, version = 0;
    int32_t fileCx = 0, fileCz = 0;
    if (!readU32(in, magic) || magic != COLUMN_MAGIC)
        return false;
    if (!readU32(in, version) || version != COLUMN_VERSION)
        return false;
    if (!readI32(in, fileCx) || !readI32(in, fileCz))
        return false;
    if (fileCx != cx || fileCz != cz)
        return false;

    constexpr int N = CHUNK_VOLUME * CHUNK_SECTIONS;
    std::vector<uint8_t> ids(N);
    std::vector<uint8_t> metas(N);
    if (!in.read(reinterpret_cast<char*>(ids.data()), N))
        return false;
    if (!in.read(reinterpret_cast<char*>(metas.data()), N))
        return false;

    unpackChunk(chunk, ids.data(), metas.data());
    return true;
}

bool saveColumn(const std::string& worldDirectory, int cx, int cz, const Chunk& chunk) {
    std::error_code ec;
    fs::create_directories(worldDirectory, ec);

    constexpr int N = CHUNK_VOLUME * CHUNK_SECTIONS;
    std::vector<uint8_t> ids(N);
    std::vector<uint8_t> metas(N);
    packChunk(chunk, ids.data(), metas.data());

    const std::string path = columnPath(worldDirectory, cx, cz);
    const std::string tmp = path + ".tmp";
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    writeU32(out, COLUMN_MAGIC);
    writeU32(out, COLUMN_VERSION);
    writeI32(out, cx);
    writeI32(out, cz);
    out.write(reinterpret_cast<const char*>(ids.data()), N);
    out.write(reinterpret_cast<const char*>(metas.data()), N);
    out.flush();
    if (!out)
        return false;
    out.close();

    fs::rename(tmp, path, ec);
    if (ec) {
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
    }
    return !ec;
}

bool deleteWorld(const std::string& folder) {
    std::error_code ec;
    fs::remove_all(worldDir(folder), ec);
    return !ec;
}

bool worldExists(const std::string& folder) {
    return !folder.empty() && fs::exists(worldDatPath(folder));
}

bool loadSettings(Config& config) {
    std::ifstream in("saves/settings.cfg");
    if (!in)
        return false;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
            continue;
        if (key == "Fov") {
            iss >> config.fov;
        } else if (key == "RenderDistance") {
            iss >> config.renderDistance;
        } else if (key == "MouseSensitivity") {
            iss >> config.mouseSensitivity;
        } else if (key == "Vsync") {
            int v = 1;
            iss >> v;
            config.vsync = v != 0;
        } else if (key == "SunMoon") {
            int v = 1;
            iss >> v;
            config.showSunMoon = v != 0;
        } else if (key == "ShowSun" || key == "ShowMoon") {
            // Old split toggles: either off means celestial lighting off.
            int v = 1;
            iss >> v;
            if (v == 0)
                config.showSunMoon = false;
        } else if (key == "Fullscreen") {
            int v = 0;
            iss >> v;
            config.isFullscreen = v != 0;
        } else if (key == "WindowWidth") {
            iss >> config.windowX;
        } else if (key == "WindowHeight") {
            iss >> config.windowY;
        } else if (key == "WindowPosX") {
            iss >> config.windowPosX;
        } else if (key == "WindowPosY") {
            iss >> config.windowPosY;
        } else if (key == "LastWorld") {
            iss >> config.lastWorld;
        }
    }

    config.fov = clampVal(config.fov, 60, 110);
    config.renderDistance = clampVal(config.renderDistance, 4, 16);
    config.mouseSensitivity = clampVal(config.mouseSensitivity, 0.04f, 0.30f);
    if (config.windowX < 640)
        config.windowX = 640;
    if (config.windowY < 360)
        config.windowY = 360;
    return true;
}

bool saveSettings(const Config& config) {
    ensureSavesDirs();
    std::ofstream out("saves/settings.cfg", std::ios::trunc);
    if (!out)
        return false;
    out << "Version 1\n";
    out << "Fov " << config.fov << "\n";
    out << "RenderDistance " << config.renderDistance << "\n";
    out << "MouseSensitivity " << config.mouseSensitivity << "\n";
    out << "Vsync " << (config.vsync ? 1 : 0) << "\n";
    out << "SunMoon " << (config.showSunMoon ? 1 : 0) << "\n";
    out << "Fullscreen " << (config.isFullscreen ? 1 : 0) << "\n";
    out << "WindowWidth " << config.windowX << "\n";
    out << "WindowHeight " << config.windowY << "\n";
    out << "WindowPosX " << config.windowPosX << "\n";
    out << "WindowPosY " << config.windowPosY << "\n";
    out << "LastWorld " << config.lastWorld << "\n";
    return static_cast<bool>(out);
}

int32_t randomSeed() {
    try {
        std::random_device rd;
        const uint32_t a = rd();
        const uint32_t b = rd();
        if (a == 0 && b == 0)
            throw std::runtime_error("random_device");
        return static_cast<int32_t>(a ^ (b << 1));
    } catch (...) {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        return static_cast<int32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    }
}

bool parseSeed(const std::string& text, int32_t& out) {
    if (text.empty() || text == "-")
        return false;
    errno = 0;
    char* end = nullptr;
    const long v = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0')
        return false;
    if (errno == ERANGE)
        return false;
    if (v < static_cast<long>(INT32_MIN) || v > static_cast<long>(INT32_MAX))
        return false;
    out = static_cast<int32_t>(v);
    return true;
}

std::string formatTime(int64_t unixSeconds) {
    if (unixSeconds <= 0)
        return "-";
    const std::time_t t = static_cast<std::time_t>(unixSeconds);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm) == 0)
        return "-";
    return buf;
}

} // namespace WorldSave
