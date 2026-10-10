#ifndef VOXELSPIRE_CORE_SETTINGS_WATER_HPP
#define VOXELSPIRE_CORE_SETTINGS_WATER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include "../../physics/fluid_flow.hpp"
#include "../../physics/fluid_resistance.hpp"

namespace voxelspire {

enum class FluidModel : std::uint8_t { None = 0, Linear, TickDamping, Quadratic };
enum class FlowModel  : std::uint8_t { Still = 0, Minecraft, Realistic };
enum class DisplaceBy : std::uint8_t { Weight = 0, BodySize };

struct WaveSettings {
    bool   enabled      = false;
    double calm_height  = 0.03;
    double storm_height = 0.35;
    double wavelength   = 18.0;
    double speed        = 1.0;
    bool   overtop      = true;
    double overtop_rate = 1.0;
};

struct WaterSettings {
    static constexpr const char* STILL     = "still";
    static constexpr const char* MINECRAFT = "minecraft";
    static constexpr const char* FLOWING   = "flowing";
    static constexpr const char* REALISTIC = "realistic";
    static constexpr const char* ULTRA     = "ultra";
 
    static constexpr double DEFAULT_LINEAR_DRAG    = 4.0;
    static constexpr int    DEFAULT_UPDATES        = 4096;
    static constexpr int    ULTRA_UPDATES          = 8192;
    static constexpr double DEFAULT_BUOYANCY       = 0.85;
    static constexpr double DEFAULT_SINK_SPEED     = 2.0;
    static constexpr double DEFAULT_CURRENT_SPEED  = 2.5;
    static constexpr double DEFAULT_CURRENT_PUSH   = 12.0;
    static constexpr double DEFAULT_WADE_SLOWDOWN  = 0.5;
    static constexpr double DEFAULT_FALL_BREAK     = 3.0;
    static constexpr double STILL_WADE_SLOWDOWN    = 0.3;
    static constexpr double MINECRAFT_FALL_BREAK   = 1.0;
    static constexpr double REALISTIC_WADE         = 0.6;
    static constexpr double ULTRA_WADE             = 0.7;
    static constexpr double ULTRA_INTERVAL         = 0.05;
    static constexpr double ULTRA_MIN_DEPTH        = 1.0 / 64.0;
 
    std::string name = REALISTIC;
 
    FlowModel    flow               = FlowModel::Realistic;
    double       minecraft_interval = MinecraftFluid::DEFAULT_INTERVAL;
    int          flow_spread        = MinecraftFluid::DEFAULT_SPREAD;
    int          slope_search       = MinecraftFluid::DEFAULT_SLOPE_SEARCH;
    bool         infinite_sources   = true;
    double       realistic_interval = RealisticFluid::DEFAULT_INTERVAL;
    double       min_depth          = RealisticFluid::DEFAULT_MIN_DEPTH;
    bool         seek_drops         = true;
    int          drop_search        = RealisticFluid::DEFAULT_DROP_SEARCH;
    bool         displacement       = false;
    DisplaceBy   displace_by        = DisplaceBy::Weight;
    bool         splashes           = false;
    double       splash             = 1.0;
    int          updates            = DEFAULT_UPDATES;
 
    WaveSettings waves;
 
    bool         rain_fills         = false;
    double       rain_fill          = 0.12;
    double       soak               = 0.25;
    double       evaporation        = 0.05;
    double       rain_reach         = 64.0;
 
    FluidModel   resistance         = FluidModel::Quadratic;
    double       fluid_density      = QuadraticFluidDrag::WATER_DENSITY;
    double       drag_coefficient   = QuadraticFluidDrag::DRAG_COEFFICIENT;
    double       linear_drag        = DEFAULT_LINEAR_DRAG;
    double       tick_damping       = TickFluidDamping::MINECRAFT_FACTOR;
    double       damping_ticks      = TickFluidDamping::MINECRAFT_TICKS;
 
    double       buoyancy           = DEFAULT_BUOYANCY;
    double       sink_speed         = DEFAULT_SINK_SPEED;
    double       current_speed      = DEFAULT_CURRENT_SPEED;
    double       current_push       = DEFAULT_CURRENT_PUSH;
    double       wade_slowdown      = DEFAULT_WADE_SLOWDOWN;
    double       fall_break_depth   = DEFAULT_FALL_BREAK;
 
    static WaterSettings still();
 
    static WaterSettings minecraft();
 
    static WaterSettings flowing() {
        WaterSettings w;
        w.name = FLOWING;
        w.flow = FlowModel::Minecraft;
        return w;
    }
 
    static WaterSettings realistic() {
        WaterSettings w;
        w.wade_slowdown = REALISTIC_WADE;
        return w;
    }
 
    static WaterSettings ultra();
 
    bool same_flow(const WaterSettings& o) const noexcept;
 
    std::shared_ptr<const FluidRules> rules(int sea_level = RealisticFluid::NO_SEA) const;
 
    std::shared_ptr<const FluidResistance> drag(double body_mass = QuadraticFluidDrag::BODY_MASS) const;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_WATER_HPP