#ifndef VOXELSPIRE_CORE_SETTINGS_WORLD_HPP
#define VOXELSPIRE_CORE_SETTINGS_WORLD_HPP

#include <cstdint>
#include <memory>
#include <string>
#include "../limits.hpp"
#include "../types.hpp"
#include "../../lighting/format.hpp"
#include "../../physics/air_resistance.hpp"
#include "../../physics/fluid_resistance.hpp"
#include "../../physics/fluid_flow.hpp"

namespace voxelspire {

struct WorldDefaults {
    static constexpr double gravity           = 32.0;
    static constexpr double terminal_velocity = 78.4;
    static constexpr double void_depth        = 64.0;
    static constexpr int    min_z             = -64;
    static constexpr int    max_z             = 320;
    static constexpr double fluid_buoyancy    = 0.85;
    static constexpr double fluid_sink_speed  = 2.0;
    static constexpr double current_speed     = 2.5;
    static constexpr double current_push      = 12.0;
    static constexpr double wade_slowdown     = 0.5;
    static constexpr double fall_break_depth  = 3.0;
    static constexpr int    fluid_updates     = 4096;

    static std::shared_ptr<const AirResistance> air_resistance();

    static std::shared_ptr<const FluidResistance> fluid_resistance() {
        return std::make_shared<QuadraticFluidDrag>();
    }

    static std::shared_ptr<const FluidRules> fluid_rules() {
        return std::make_shared<MinecraftFluid>();
    }
};

struct WorldSettings {
    static constexpr std::uint64_t RANDOM_SEED  = 0;
    static constexpr const char*   DEFAULT_NAME = "New World";

    std::uint64_t seed       = RANDOM_SEED;
    std::string   name       = DEFAULT_NAME;
    int    min_z             = WorldDefaults::min_z;
    int    max_z             = WorldDefaults::max_z;
    double gravity           = WorldDefaults::gravity;
    double void_depth        = WorldDefaults::void_depth;
    int    horizontal_limit  = EngineLimits::WORLD_MAX_HORIZONTAL;
    LightFormat light_format = LightFormat::Colored;

    std::shared_ptr<const AirResistance>   air_resistance   = WorldDefaults::air_resistance();
    std::shared_ptr<const FluidResistance> fluid_resistance = WorldDefaults::fluid_resistance();
    std::shared_ptr<const FluidRules>      fluid_rules      = WorldDefaults::fluid_rules();
    double current_speed    = WorldDefaults::current_speed;
    double current_push     = WorldDefaults::current_push;
    double wade_slowdown    = WorldDefaults::wade_slowdown;
    double fall_break_depth = WorldDefaults::fall_break_depth;
    int    fluid_updates    = WorldDefaults::fluid_updates;
    double fluid_buoyancy   = WorldDefaults::fluid_buoyancy;
    double fluid_sink_speed = WorldDefaults::fluid_sink_speed;

    double fall_height() const noexcept { return static_cast<double>(max_z - min_z) + void_depth; }

    WorldSettings validated() const;
};

struct SaveSettings {
    double      autosave_minutes = 5.0;
    std::string folder           = "worlds";
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_WORLD_HPP