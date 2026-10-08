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
        : m_root(std::move(root)), m_biomes(std::move(biomes)), m_water(std::move(water)) {
        WorldPages::register_all(m_pages, m_biomes);
        SettingsPages::register_world(m_pages, SettingsHooks{}, m_water);
    }

    WorldStore(const WorldStore&) = delete;
    WorldStore& operator=(const WorldStore&) = delete;

    const std::filesystem::path& root()   const noexcept { return m_root; }
    const SettingsRegistry&      pages()  const noexcept { return m_pages; }
    const BiomeRegistry&         biomes() const noexcept { return m_biomes; }
    const WaterPresets&          water()  const noexcept { return m_water; }

    std::filesystem::path folder_path(const std::string& folder) const { return m_root / folder; }

    std::vector<WorldRecord> list() const {
        std::vector<WorldRecord> out;
        std::error_code ec;
        if (!std::filesystem::is_directory(m_root, ec)) return out;

        for (const auto& entry : std::filesystem::directory_iterator(m_root, ec)) {
            if (!entry.is_directory(ec)) continue;
            const std::string folder = entry.path().filename().string();
            GameSettings scratch;
            WorldRecord record;
            if (load(folder, scratch, &record)) out.push_back(record);
        }

        std::sort(out.begin(), out.end(), [](const WorldRecord& a, const WorldRecord& b) {
            if (a.played != b.played) return a.played > b.played;
            return a.name < b.name;
        });

        return out;
    }

    std::optional<WorldRecord> create(GameSettings& options) {
        std::error_code ec;
        std::filesystem::create_directories(m_root, ec);
        WorldRecord record;
        record.name    = options.world.name.empty() ? std::string(WorldSettings::DEFAULT_NAME) : options.world.name;
        record.folder  = unique_folder(record.name);
        record.seed    = SecureRandom::seed();
        record.created = now();
        record.played  = record.created;
        options.world.seed = record.seed;
        if (!std::filesystem::create_directories(folder_path(record.folder), ec) || ec) return std::nullopt;
        if (!save(record, options)) return std::nullopt;
        return record;
    }

    bool load(const std::string& folder, GameSettings& into, WorldRecord* record = nullptr, WorldState* state = nullptr) const {
        std::ifstream in(folder_path(folder) / OPTIONS_FILE);
        if (!in) return false;
        const SettingsFile::Values values = SettingsFile::read(in);
        if (state) *state = WorldStateFormat::read(values);
        GameSettings defaults = into;
        std::vector<SettingsTab> tabs = m_pages.build(into, defaults);
        SettingsFile::apply(tabs, values);
        into.world.seed = number(values, SEED_KEY);
        if (into.world.seed == WorldSettings::RANDOM_SEED) return false;

        if (record) {
            record->folder  = folder;
            record->name    = into.world.name;
            record->seed    = into.world.seed;
            record->created = static_cast<std::int64_t>(number(values, CREATED_KEY));
            record->played  = static_cast<std::int64_t>(number(values, PLAYED_KEY));
        }

        return true;
    }

    bool save(const WorldRecord& record, GameSettings& options, const WorldState* state = nullptr) const {
        const std::filesystem::path path = folder_path(record.folder) / OPTIONS_FILE;
        const std::filesystem::path temp = path.string() + TEMP_SUFFIX;
        WorldState kept;

        if (!state) {
            std::ifstream old(path);
            if (old) kept = WorldStateFormat::read(SettingsFile::read(old));
            state = &kept;
        }

        GameSettings defaults = options;
        std::vector<SettingsTab> tabs = m_pages.build(options, defaults);
        
        {
            std::ofstream out(temp, std::ios::trunc);
            if (!out) return false;
            out << "[Save]\n";
            out << SEED_KEY << " = " << record.seed << "\n";
            out << CREATED_KEY << " = " << record.created << "\n";
            out << PLAYED_KEY << " = " << record.played << "\n\n";
            WorldStateFormat::write(out, *state);
            SettingsFile::write(tabs, out);
            if (!out) return false;
        }

        std::error_code ec;
        std::filesystem::rename(temp, path, ec);
        return !ec;
    }

    bool touch(const std::string& folder) const {
        GameSettings options;
        WorldRecord record;
        WorldState state;
        if (!load(folder, options, &record, &state)) return false;
        record.played = now();
        return save(record, options, &state);
    }

    bool save_state(const std::string& folder, const WorldState& state) const {
        GameSettings options;
        WorldRecord record;
        if (!load(folder, options, &record)) return false;
        record.played = now();
        return save(record, options, &state);
    }

    bool save_options(const std::string& folder, GameSettings options, const WorldState& state) const {
        GameSettings stored;
        WorldRecord record;
        if (!load(folder, stored, &record)) return false;
        record.played = now();
        options.world.seed = record.seed;
        return save(record, options, &state);
    }

    bool remove(const std::string& folder) const {
        if (folder.empty() || folder.find("..") != std::string::npos) return false;
        std::error_code ec;
        std::filesystem::remove_all(folder_path(folder), ec);
        return !ec;
    }

    static std::int64_t now() noexcept {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

private:
    static std::uint64_t number(const SettingsFile::Values& values, const char* key) {
        auto it = values.find(key);
        if (it == values.end()) return 0;
        return std::strtoull(it->second.c_str(), nullptr, DECIMAL);
    }

    std::string unique_folder(const std::string& name) const {
        std::string base;

        for (char ch : name) {
            const unsigned char c = static_cast<unsigned char>(ch);
            if (std::isalnum(c)) base += static_cast<char>(std::tolower(c));
            else if (!base.empty() && base.back() != '_') base += '_';
            if (base.size() >= MAX_FOLDER) break;
        }

        while (!base.empty() && base.back() == '_') base.pop_back();
        if (base.empty()) base = FALLBACK_NAME;
        std::string folder = base;
        std::error_code ec;
        for (int n = FIRST_SUFFIX; std::filesystem::exists(folder_path(folder), ec); ++n) folder = base + "_" + std::to_string(n);
        return folder;
    }

    static constexpr int DECIMAL      = 10;
    static constexpr int FIRST_SUFFIX = 2;

    std::filesystem::path m_root;
    BiomeRegistry         m_biomes;
    WaterPresets          m_water;
    SettingsRegistry      m_pages;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_WORLD_STORE_HPP