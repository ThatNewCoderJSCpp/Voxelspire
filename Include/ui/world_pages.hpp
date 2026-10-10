#ifndef VOXELSPIRE_UI_WORLD_PAGES_HPP
#define VOXELSPIRE_UI_WORLD_PAGES_HPP

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../world/biome.hpp"
#include "settings_registry.hpp"

namespace voxelspire {

struct WorldTabs {
    static constexpr const char* WORLD  = "world";
    static constexpr const char* LAND   = "land";
    static constexpr const char* BIOMES = "biomes";
    static constexpr const char* RIVERS = "rivers";
    static constexpr const char* CAVES  = "caves";
    static constexpr const char* BIOME  = "biome.";
};

struct WorldGroups {
    static constexpr const char* WORLD  = "World";
    static constexpr const char* BIOMES = "Biomes";
};

struct WorldOptionLimits {
    static constexpr std::size_t NAME_LENGTH = 48;
    static constexpr int         MIN_HEIGHT  = 2 * EngineLimits::CHUNK_SIZE;

    static constexpr Bounds bottom { EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - MIN_HEIGHT };
    static constexpr Bounds top    { EngineLimits::WORLD_MIN_Z + MIN_HEIGHT, EngineLimits::WORLD_MAX_Z };
    static constexpr Bounds radius { EngineLimits::CHUNK_SIZE, EngineLimits::WORLD_MAX_HORIZONTAL };
};

class WorldPages {
public:
    using BlockList = std::vector<Identifier>;

    static BlockList surface_blocks();

    static void register_all(SettingsRegistry& registry, const BiomeRegistry& biomes, BlockList blocks = surface_blocks());

private:
    static void world(SettingsPage& p);

    static void land(SettingsPage& p);

    static void biome_page(SettingsPage& p, const BiomeRegistry& list);

    static void biome_tab(SettingsPage& p, const Biome& biome, const std::vector<std::string>& palette);

    template <typename Get, typename Edit, typename Tidy>
    static SettingsPage& optional_number(SettingsPage& p, const std::string& label, const std::string& description, Bounds limits, double normal,
                                         Get get, Edit edit, Tidy tidy, std::optional<double> BiomeOptions::*member) {
        return p.custom_decimal(label, description,
            [get, member, normal] { return (get().*member).value_or(normal); },
            [edit, tidy, member, normal](double v) { if (v == normal) (edit().*member).reset(); else edit().*member = v; tidy(); },
            limits
        ).defaults([edit, tidy, member] { (edit().*member).reset(); tidy(); }, [get, member] { return !(get().*member); });
    }

    template <typename Get, typename Edit, typename Tidy>
    static SettingsPage& number(SettingsPage& p, const std::string& label, const std::string& description, Bounds limits, double normal,
                                Get get, Edit edit, Tidy tidy, double BiomeOptions::*member) {
        return p.custom_decimal(label, description,
            [get, member] { return get().*member; },
            [edit, tidy, member](double v) { edit().*member = v; tidy(); },
            limits
        ).defaults([edit, tidy, member, normal] { edit().*member = normal; tidy(); }, [get, member, normal] { return get().*member == normal; });
    }

    template <typename Get, typename Edit, typename Tidy>
    static SettingsPage& block(SettingsPage& p, const std::string& label, const std::string& description, const std::vector<std::string>& palette,
                               Identifier normal, Get get, Edit edit, Tidy tidy, std::string BiomeOptions::*member) {
        const std::string fallback = normal.str();
        auto current = [get, member, fallback] { const std::string& v = get().*member; return v.empty() ? fallback : v; };

        auto options = [palette, current] {
            std::vector<std::string> list = palette;
            const std::string now = current();
            if (std::find(list.begin(), list.end(), now) == list.end()) list.push_back(now);
            return list;
        };

        return p.custom_choice(label, description, options,
            [options, current] {
                const std::vector<std::string> list = options();
                return static_cast<int>(std::find(list.begin(), list.end(), current()) - list.begin());
            },
            [options, edit, tidy, member, fallback](int i) {
                const std::vector<std::string> list = options();
                if (i < 0 || static_cast<std::size_t>(i) >= list.size()) return;
                edit().*member = list[static_cast<std::size_t>(i)] == fallback ? std::string() : list[static_cast<std::size_t>(i)];
                tidy();
            }
        ).defaults([edit, tidy, member] { (edit().*member).clear(); tidy(); }, [get, member] { return (get().*member).empty(); });
    }

    static std::vector<std::string> names(const BlockList& blocks);

    static void tidy_all(GameSettings& g);

    static void caves(SettingsPage& p);

    static void rivers(SettingsPage& p);
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_WORLD_PAGES_HPP