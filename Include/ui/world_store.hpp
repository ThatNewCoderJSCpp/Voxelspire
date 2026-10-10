#ifndef VOXELSPIRE_UI_WORLD_STORE_HPP
#define VOXELSPIRE_UI_WORLD_STORE_HPP

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>
#include "../core/random.hpp"
#include "../world/world_save.hpp"
#include "settings_file.hpp"
#include "settings_pages.hpp"
#include "world_pages.hpp"

namespace voxelspire {

struct WorldRecord {
    std::string   folder;
    std::string   name;
    std::uint64_t seed    = 0;
    std::int64_t  created = 0;
    std::int64_t  played  = 0;
};

class WorldStore {
public:
    static constexpr const char* OPTIONS_FILE  = "world.cfg";
    static constexpr const char* TEMP_SUFFIX   = ".tmp";
    static constexpr const char* FALLBACK_NAME = "world";
    static constexpr const char* SEED_KEY      = "save.seed";
    static constexpr const char* CREATED_KEY   = "save.created";
    static constexpr const char* PLAYED_KEY    = "save.played";
    static constexpr std::size_t MAX_FOLDER    = 40;

    explicit WorldStore(std::filesystem::path root, BiomeRegistry biomes = BiomeRegistry::builtin(), WaterPresets water = WaterPresets::builtin())
;

    WorldStore(const WorldStore&) = delete;
    WorldStore& operator=(const WorldStore&) = delete;

    const std::filesystem::path& root()   const noexcept { return m_root; }
    const SettingsRegistry&      pages()  const noexcept { return m_pages; }
    const BiomeRegistry&         biomes() const noexcept { return m_biomes; }
    const WaterPresets&          water()  const noexcept { return m_water; }

    std::filesystem::path folder_path(const std::string& folder) const { return m_root / folder; }

    std::vector<WorldRecord> list() const;

    std::optional<WorldRecord> create(GameSettings& options);

    bool load(const std::string& folder, GameSettings& into, WorldRecord* record = nullptr, WorldState* state = nullptr) const;

    bool save(const WorldRecord& record, GameSettings& options, const WorldState* state = nullptr) const;

    bool touch(const std::string& folder) const;

    bool save_state(const std::string& folder, const WorldState& state) const;

    bool save_options(const std::string& folder, GameSettings options, const WorldState& state) const;

    bool remove(const std::string& folder) const;

    static std::int64_t now() noexcept;

private:
    static std::uint64_t number(const SettingsFile::Values& values, const char* key);

    std::string unique_folder(const std::string& name) const;

    static constexpr int DECIMAL      = 10;
    static constexpr int FIRST_SUFFIX = 2;

    std::filesystem::path m_root;
    BiomeRegistry         m_biomes;
    WaterPresets          m_water;
    SettingsRegistry      m_pages;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_WORLD_STORE_HPP