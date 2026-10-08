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
 
    static WaterSettings still() {
        WaterSettings w;
        w.name          = STILL;
        w.flow          = FlowModel::Still;
        w.resistance    = FluidModel::Linear;
        w.current_speed = 0.0;
        w.current_push  = 0.0;
        w.wade_slowdown = STILL_WADE_SLOWDOWN;
        return w;
    }
 
    static WaterSettings minecraft() {
        WaterSettings w;
        w.name             = MINECRAFT;
        w.flow             = FlowModel::Minecraft;
        w.resistance       = FluidModel::TickDamping;
        w.fall_break_depth = MINECRAFT_FALL_BREAK;
        return w;
    }
 
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
 
    static WaterSettings ultra() {
        WaterSettings w;
        w.name               = ULTRA;
        w.realistic_interval = ULTRA_INTERVAL;
        w.min_depth          = ULTRA_MIN_DEPTH;
        w.drop_search        = RealisticFluid::MAX_DROP_SEARCH;
        w.updates            = ULTRA_UPDATES;
        w.wade_slowdown      = ULTRA_WADE;
        return w;
    }
 
    bool same_flow(const WaterSettings& o) const noexcept {
        if (flow != o.flow) return false;
 
        switch (flow) {
            case FlowModel::Still:     return true;
            case FlowModel::Minecraft: return minecraft_interval == o.minecraft_interval && flow_spread == o.flow_spread && slope_search == o.slope_search && infinite_sources == o.infinite_sources;
            case FlowModel::Realistic: return realistic_interval == o.realistic_interval && min_depth == o.min_depth && seek_drops == o.seek_drops && drop_search == o.drop_search
                                           && displacement == o.displacement && splashes == o.splashes && splash == o.splash;
        }
 
        return false;
    }
 
    std::shared_ptr<const FluidRules> rules(int sea_level = RealisticFluid::NO_SEA) const {
        switch (flow) {
            case FlowModel::Still:     return std::make_shared<StillFluid>();
            case FlowModel::Minecraft: return std::make_shared<MinecraftFluid>(minecraft_interval, flow_spread, slope_search, infinite_sources);
            case FlowModel::Realistic: return std::make_shared<RealisticFluid>(realistic_interval, min_depth, seek_drops, drop_search, displacement, splashes ? splash : 0.0, sea_level);
        }
 
        return std::make_shared<StillFluid>();
    }
 
    std::shared_ptr<const FluidResistance> drag(double body_mass = QuadraticFluidDrag::BODY_MASS) const {
        switch (resistance) {
            case FluidModel::None:        return std::make_shared<NoFluidResistance>();
            case FluidModel::Linear:      return std::make_shared<LinearFluidDrag>(linear_drag);
            case FluidModel::TickDamping: return std::make_shared<TickFluidDamping>(tick_damping, damping_ticks);
            case FluidModel::Quadratic:   return std::make_shared<QuadraticFluidDrag>(fluid_density, drag_coefficient, body_mass);
        }
 
        return std::make_shared<NoFluidResistance>();
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_WATER_HPP