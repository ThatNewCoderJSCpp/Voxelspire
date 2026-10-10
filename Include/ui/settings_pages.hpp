#ifndef VOXELSPIRE_UI_SETTINGS_PAGES_HPP
#define VOXELSPIRE_UI_SETTINGS_PAGES_HPP

#include <functional>
#include <string>
#include <vector>
#include "../core/presets.hpp"
#include "settings_registry.hpp"

namespace voxelspire {

struct SettingsHooks {
    std::function<double()>     hour;
    std::function<void(double)> set_hour;
    std::function<void()>       save;
    std::function<void()>       reload;
    std::function<void()>       respawn;
    std::function<void()>       save_world;
};

struct SettingsGroups {
    static constexpr const char* GAME        = "Game";
    static constexpr const char* GRAPHICS    = "Graphics";
    static constexpr const char* PERFORMANCE = "Performance";
    static constexpr const char* PLAYER      = "Player";
    static constexpr const char* GAMEPLAY    = "Gameplay";
    static constexpr const char* SKY         = "Sky";
};

struct SettingsTabs {
    static constexpr const char* GENERAL     = "general";
    static constexpr const char* CONTROLS    = "controls";
    static constexpr const char* CHARACTER   = "character";
    static constexpr const char* PHYSICS     = "physics";
    static constexpr const char* WATER       = "water";
    static constexpr const char* TIME        = "time_sky";
    static constexpr const char* CELESTIAL   = "sun_moon_stars";
    static constexpr const char* SEASONS     = "seasons";
    static constexpr const char* WEATHER     = "weather";
    static constexpr const char* LIGHTING    = "lighting";
    static constexpr const char* SHADOWS     = "shadows";
    static constexpr const char* ATMOSPHERE  = "atmosphere";
    static constexpr const char* REFLECTIONS = "reflections_water";
    static constexpr const char* RENDERING   = "rendering";
    static constexpr const char* SIMULATION  = "simulation";
    static constexpr const char* HUD         = "hud";
    static constexpr const char* SURVIVAL    = "survival";
    static constexpr const char* STAMINA     = "stamina_load";
    static constexpr const char* INVENTORY   = "inventory";
    static constexpr const char* ITEMS       = "items";
};

class SettingsPages {
public:
    static constexpr const char* CUSTOM = "custom";

    static void register_personal(SettingsRegistry& registry, const SettingsHooks& hooks, const LightingPresets& presets);

    static void register_player(SettingsRegistry& registry);

    static void register_world(SettingsRegistry& registry, const SettingsHooks& hooks, const WaterPresets& water_presets);

    static void register_all(SettingsRegistry& registry, const SettingsHooks& hooks, const LightingPresets& presets, const WaterPresets& water_presets) {
        register_personal(registry, hooks, presets);
        register_world(registry, hooks, water_presets);
    }

private:
    static constexpr double BYTES_PER_MB = 1024.0 * 1024.0;

    static void mark_custom(SettingsPage& p) {
        GameSettings* g = &p.live();
        p.touching([g] { g->lighting.name = CUSTOM; });
    }

    static void mark_water_custom(SettingsPage& p) {
        GameSettings* g = &p.live();
        p.touching([g] { g->water.name = CUSTOM; });
    }

    static void general(SettingsPage& p, const SettingsHooks& hooks);

    static void controls(SettingsPage& p);

    static bool movement(Action a) noexcept;

    static void character(SettingsPage& p);

    static void physics(SettingsPage& p);

    static void water(SettingsPage& p, const WaterPresets& presets);

    static void time(SettingsPage& p, const SettingsHooks& hooks);

    static void seasons(SettingsPage& p);

    static void weather(SettingsPage& p);

    static void celestial(SettingsPage& p);

    static void lighting(SettingsPage& p, const LightingPresets& presets);

    static void shadows(SettingsPage& p);

    static void power_choice(
        SettingsPage& p, 
        const std::string& label, 
        const std::string& description,
        unsigned int LightingSettings::*member, 
        std::vector<unsigned int> values
    );

    static void atmosphere(SettingsPage& p);

    static void reflections(SettingsPage& p);

    static void rendering(SettingsPage& p);

    static void simulation(SettingsPage& p);

    static void need(SettingsPage& p, NeedSettings SurvivalSettings::*which, const char* name, const char* thing, const char* food);

    static void survival(SettingsPage& p);

    static void activity(SettingsPage& p, ActivityCost ExertionSettings::*which, const std::string& name, const std::string& doing, bool uses_stamina);

    static void stamina(SettingsPage& p);

    static void inventory(SettingsPage& p);

    static void items(SettingsPage& p);

    static void hud(SettingsPage& p);
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_PAGES_HPP