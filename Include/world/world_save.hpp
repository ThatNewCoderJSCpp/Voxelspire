#ifndef VOXELSPIRE_WORLD_WORLD_SAVE_HPP
#define VOXELSPIRE_WORLD_WORLD_SAVE_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

struct SavedStack {
    int         slot  = 0;
    std::string item;
    int         count = 0;
};

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
    bool         has_vitals        = false;
    bool         dead              = false;
    double       health            = 0.0;
    double       hunger            = 0.0;
    double       thirst            = 0.0;
    double       stamina           = 0.0;
    double       breath            = 0.0;
    double       stomach_bulk      = 0.0;
    double       stomach_energy    = 0.0;
    double       stomach_water     = 0.0;
    bool         has_inventory     = false;
    int          selected_slot     = 0;

    std::vector<SavedStack> inventory;
    std::vector<SavedStack> stack_sizes;
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
    static constexpr const char* HEALTH       = "state.vitals.health";
    static constexpr const char* HUNGER       = "state.vitals.hunger";
    static constexpr const char* THIRST       = "state.vitals.thirst";
    static constexpr const char* STAMINA      = "state.vitals.stamina";
    static constexpr const char* BREATH       = "state.vitals.breath";
    static constexpr const char* DEAD         = "state.vitals.dead";
    static constexpr const char* STOMACH_BULK = "state.vitals.stomach.bulk";
    static constexpr const char* STOMACH_FOOD = "state.vitals.stomach.energy";
    static constexpr const char* STOMACH_WET  = "state.vitals.stomach.water";
    static constexpr const char* SELECTED     = "state.inventory.selected";
    static constexpr const char* SLOT_PREFIX  = "state.inventory.slot.";
    static constexpr const char* STACK_PREFIX = "state.items.stack.";

    static void write(std::ostream& out, const WorldState& s);

    static WorldState read(const Values& values);

private:
    static constexpr std::size_t BUFFER = 96;
    static constexpr int         DECIMAL = 10;

    static bool stack_value(const std::string& text, SavedStack& out);

    static void read_inventory(const Values& values, WorldState& s);
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

    bool save_column(const ColumnPos& col, const std::vector<const Chunk*>& chunks) const;

    std::vector<std::unique_ptr<Chunk>> load_column(const ColumnPos& col) const;

private:
    static constexpr std::uint16_t NOT_MAPPED = 0xFFFF;
    static constexpr int           BYTE_BITS  = 8;
    static constexpr int           WORD_BYTES = 4;
    static constexpr std::uint32_t BYTE_MASK  = 0xFF;

    std::filesystem::path file_for(const ColumnPos& c) const {
        return m_dir / (std::to_string(c.x) + "." + std::to_string(c.y) + EXTENSION);
    }

    static void put16(std::ostream& out, std::uint16_t v);

    static void put32(std::ostream& out, std::uint32_t v) {
        for (int i = 0; i < WORD_BYTES; ++i) out.put(static_cast<char>((v >> (i * BYTE_BITS)) & BYTE_MASK));
    }

    static std::uint16_t get16(std::istream& in);

    static std::uint32_t get32(std::istream& in);

    static void write_runs(std::ostream& out, const Chunk& c, const std::vector<std::uint16_t>& remap);

    static bool read_runs(std::istream& in, Chunk& c, const std::vector<BlockId>& palette);

    std::filesystem::path m_dir;
    const BlockRegistry&  m_registry;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_WORLD_SAVE_HPP