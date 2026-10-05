#ifndef VOXELSPIRE_WORLD_WORLD_SAVE_HPP
#define VOXELSPIRE_WORLD_WORLD_SAVE_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include "../block/block_registry.hpp"
#include "chunk.hpp"

namespace voxelspire {

struct WorldState {
    bool         has_player        = false;
    vector3d     position{};
    double       yaw               = 0.0;
    double       pitch             = 0.0;
    double       time              = 0.0;
    std::int64_t day               = 0;
    bool         has_time          = false;
    bool         has_weather       = false;
    int          weather_kind      = 0;
    double       weather_intensity = 0.0;
    double       weather_target    = 0.0;
    double       weather_remaining = 0.0;
    bool         weather_hail      = false;
};

struct WorldStateFormat {
    using Values = std::unordered_map<std::string, std::string>;

    static constexpr const char* SECTION      = "[State]";
    static constexpr const char* PLAYER_X     = "state.player.x";
    static constexpr const char* PLAYER_Y     = "state.player.y";
    static constexpr const char* PLAYER_Z     = "state.player.z";
    static constexpr const char* PLAYER_YAW   = "state.player.yaw";
    static constexpr const char* PLAYER_PITCH = "state.player.pitch";
    static constexpr const char* CLOCK_TIME   = "state.clock.time";
    static constexpr const char* CLOCK_DAY    = "state.clock.day";
    static constexpr const char* WEATHER_KIND = "state.weather.kind";
    static constexpr const char* WEATHER_NOW  = "state.weather.intensity";
    static constexpr const char* WEATHER_GOAL = "state.weather.target";
    static constexpr const char* WEATHER_LEFT = "state.weather.remaining";
    static constexpr const char* WEATHER_HAIL = "state.weather.hail";

    static void write(std::ostream& out, const WorldState& s) {
        if (!s.has_player && !s.has_time && !s.has_weather) return;
        char buf[BUFFER];

        auto line = [&](const char* key, double v) {
            std::snprintf(buf, sizeof(buf), "%s = %.17g\n", key, v);
            out << buf;
        };

        out << SECTION << "\n";

        if (s.has_player) {
            line(PLAYER_X, s.position.x);
            line(PLAYER_Y, s.position.y);
            line(PLAYER_Z, s.position.z);
            line(PLAYER_YAW, s.yaw);
            line(PLAYER_PITCH, s.pitch);
        }

        if (s.has_time) {
            line(CLOCK_TIME, s.time);
            out << CLOCK_DAY << " = " << s.day << "\n";
        }

        if (s.has_weather) {
            line(WEATHER_KIND, s.weather_kind);
            line(WEATHER_NOW, s.weather_intensity);
            line(WEATHER_GOAL, s.weather_target);
            line(WEATHER_LEFT, s.weather_remaining);
            line(WEATHER_HAIL, s.weather_hail ? 1.0 : 0.0);
        }

        out << "\n";
    }

    static WorldState read(const Values& values) {
        WorldState s;

        auto number = [&](const char* key, double& out) {
            auto it = values.find(key);
            if (it == values.end()) return false;
            char* end = nullptr;
            const double v = std::strtod(it->second.c_str(), &end);
            if (end == it->second.c_str() || !std::isfinite(v)) return false;
            out = v;
            return true;
        };

        s.has_player = number(PLAYER_X, s.position.x) && number(PLAYER_Y, s.position.y) && number(PLAYER_Z, s.position.z);
        number(PLAYER_YAW, s.yaw);
        number(PLAYER_PITCH, s.pitch);
        double day = 0.0;
        s.has_time = number(CLOCK_TIME, s.time) && number(CLOCK_DAY, day);
        s.day = static_cast<std::int64_t>(day);
        double kind = 0.0, hail = 0.0;
        s.has_weather = number(WEATHER_KIND, kind) && number(WEATHER_NOW, s.weather_intensity) && number(WEATHER_GOAL, s.weather_target) && number(WEATHER_LEFT, s.weather_remaining);
        number(WEATHER_HAIL, hail);
        s.weather_kind = static_cast<int>(kind);
        s.weather_hail = hail != 0.0;
        return s;
    }

private:
    static constexpr std::size_t BUFFER = 96;
};

class ChunkArchive {
public:
    static constexpr std::uint32_t MAGIC        = 0x43535856u;
    static constexpr std::uint32_t VERSION      = 1;
    static constexpr const char*   FOLDER       = "chunks";
    static constexpr const char*   EXTENSION    = ".bin";
    static constexpr const char*   TEMP_SUFFIX  = ".tmp";
    static constexpr std::uint8_t  HAS_STATES   = 1;
    static constexpr std::uint16_t MAX_RUN      = 0xFFFF;

    ChunkArchive(std::filesystem::path world_folder, const BlockRegistry& registry)
        : m_dir(std::move(world_folder) / FOLDER), m_registry(registry) {}

    const std::filesystem::path& folder() const noexcept { return m_dir; }

    bool save_column(const ColumnPos& col, const std::vector<const Chunk*>& chunks) const {
        const std::filesystem::path path = file_for(col);
        std::error_code ec;

        if (chunks.empty()) {
            std::filesystem::remove(path, ec);
            return true;
        }

        std::filesystem::create_directories(m_dir, ec);
        if (ec) return false;
        std::vector<BlockId> palette;
        std::vector<std::uint16_t> remap(m_registry.size(), NOT_MAPPED);

        for (const Chunk* c : chunks)
            for (int i = 0; i < Chunk::VOLUME; ++i) {
                const BlockId id = c->data()[i];
                if (id < remap.size() && remap[id] == NOT_MAPPED) { remap[id] = static_cast<std::uint16_t>(palette.size()); palette.push_back(id); }
            }

        const std::filesystem::path temp = path.string() + TEMP_SUFFIX;
        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            if (!out) return false;
            put32(out, MAGIC);
            put32(out, VERSION);
            put32(out, static_cast<std::uint32_t>(palette.size()));

            for (BlockId id : palette) {
                const std::string& name = m_registry.get(id).identifier().str();
                put16(out, static_cast<std::uint16_t>(name.size()));
                out.write(name.data(), static_cast<std::streamsize>(name.size()));
            }

            put32(out, static_cast<std::uint32_t>(chunks.size()));

            for (const Chunk* c : chunks) {
                put32(out, static_cast<std::uint32_t>(c->pos().z));
                const bool states = c->has_states();
                out.put(static_cast<char>(states ? HAS_STATES : 0));
                write_runs(out, *c, remap);
                if (states) out.write(reinterpret_cast<const char*>(c->states()), Chunk::VOLUME);
            }

            if (!out) return false;
        }

        std::filesystem::rename(temp, path, ec);
        return !ec;
    }

    std::vector<std::unique_ptr<Chunk>> load_column(const ColumnPos& col) const {
        std::vector<std::unique_ptr<Chunk>> out;
        std::ifstream in(file_for(col), std::ios::binary);
        if (!in) return out;
        if (get32(in) != MAGIC || get32(in) != VERSION) return out;
        const std::uint32_t count = get32(in);
        std::vector<BlockId> palette;
        palette.reserve(count);

        for (std::uint32_t i = 0; i < count && in; ++i) {
            std::string name(get16(in), '\0');
            in.read(&name[0], static_cast<std::streamsize>(name.size()));
            const auto id = m_registry.find(name);
            palette.push_back(id ? *id : AIR_ID);
        }

        const std::uint32_t chunks = get32(in);

        for (std::uint32_t k = 0; k < chunks && in; ++k) {
            const int z = static_cast<std::int32_t>(get32(in));
            const bool states = (static_cast<std::uint8_t>(in.get()) & HAS_STATES) != 0;
            auto chunk = std::make_unique<Chunk>(ChunkPos{ col.x, col.y, z });
            if (!read_runs(in, *chunk, palette)) return {};

            if (states) {
                std::vector<std::uint8_t> raw(Chunk::VOLUME);
                in.read(reinterpret_cast<char*>(raw.data()), Chunk::VOLUME);
                chunk->set_states(raw);
            }

            if (!in) return {};
            chunk->mark_modified();
            out.push_back(std::move(chunk));
        }

        return out;
    }

private:
    static constexpr std::uint16_t NOT_MAPPED = 0xFFFF;
    static constexpr int           BYTE_BITS  = 8;
    static constexpr int           WORD_BYTES = 4;
    static constexpr std::uint32_t BYTE_MASK  = 0xFF;

    std::filesystem::path file_for(const ColumnPos& c) const {
        return m_dir / (std::to_string(c.x) + "." + std::to_string(c.y) + EXTENSION);
    }

    static void put16(std::ostream& out, std::uint16_t v) {
        out.put(static_cast<char>(v & BYTE_MASK));
        out.put(static_cast<char>((v >> BYTE_BITS) & BYTE_MASK));
    }

    static void put32(std::ostream& out, std::uint32_t v) {
        for (int i = 0; i < WORD_BYTES; ++i) out.put(static_cast<char>((v >> (i * BYTE_BITS)) & BYTE_MASK));
    }

    static std::uint16_t get16(std::istream& in) {
        const std::uint32_t a = static_cast<std::uint8_t>(in.get()), b = static_cast<std::uint8_t>(in.get());
        return static_cast<std::uint16_t>(a | (b << BYTE_BITS));
    }

    static std::uint32_t get32(std::istream& in) {
        std::uint32_t v = 0;
        for (int i = 0; i < WORD_BYTES; ++i) v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(in.get())) << (i * BYTE_BITS);
        return v;
    }

    static void write_runs(std::ostream& out, const Chunk& c, const std::vector<std::uint16_t>& remap) {
        const BlockId* data = c.data();
        int i = 0;

        while (i < Chunk::VOLUME) {
            const BlockId id = data[i];
            int run = 1;
            while (i + run < Chunk::VOLUME && data[i + run] == id && run < MAX_RUN) ++run;
            put16(out, static_cast<std::uint16_t>(run));
            put16(out, id < remap.size() ? remap[id] : 0);
            i += run;
        }
    }

    static bool read_runs(std::istream& in, Chunk& c, const std::vector<BlockId>& palette) {
        std::vector<BlockId> blocks(Chunk::VOLUME, AIR_ID);
        int i = 0;

        while (i < Chunk::VOLUME && in) {
            const int run = get16(in);
            const std::uint16_t index = get16(in);
            if (run <= 0 || i + run > Chunk::VOLUME) return false;
            const BlockId id = index < palette.size() ? palette[index] : AIR_ID;
            std::fill(blocks.begin() + i, blocks.begin() + i + run, id);
            i += run;
        }

        if (i != Chunk::VOLUME) return false;
        c.set_blocks(blocks);
        return true;
    }

    std::filesystem::path m_dir;
    const BlockRegistry&  m_registry;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_WORLD_SAVE_HPP