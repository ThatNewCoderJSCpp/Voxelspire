#ifndef VOXELSPIRE_PHYSICS_PHYSICS_SETTINGS_HPP
#define VOXELSPIRE_PHYSICS_PHYSICS_SETTINGS_HPP

#include <memory>
#include "air_resistance.hpp"
#include "../core/limits.hpp"
#include "fluid_resistance.hpp"
#include "fluid_flow.hpp"

namespace voxelspire {

enum class AirModel   : std::uint8_t { Keep = 0, None, TerminalCap, WorldHeight, Linear, Quadratic };
enum class FluidModel : std::uint8_t { Keep = 0, None, Linear, TickDamping, Quadratic };
enum class FlowModel  : std::uint8_t { Keep = 0, Still, Minecraft, Realistic };

struct PhysicsLimits {
    static constexpr Bounds terminal_velocity { 5.0, 400.0 };
    static constexpr Bounds fluid_density     { 100.0, 3000.0 };
    static constexpr Bounds drag_coefficient  { 0.1, 3.0 };
    static constexpr Bounds body_mass         { 10.0, 300.0 };
    static constexpr Bounds fluid_linear_drag { 0.0, 20.0 };
    static constexpr Bounds tick_damping      { 0.0, 1.0 };
    static constexpr Bounds damping_ticks     { 1.0, 100.0 };
    static constexpr Bounds flow_interval     { 0.02, 2.0 };
    static constexpr Bounds flow_spread       { 1.0, 7.0 };
    static constexpr Bounds slope_search      { 0.0, 8.0 };
    static constexpr Bounds min_depth         { 0.004, 0.5 };
};

struct PhysicsSettings {
    static constexpr double DEFAULT_TERMINAL_VELOCITY = 78.4;
    static constexpr double DEFAULT_FLUID_LINEAR      = 4.0;

    AirModel   air_model         = AirModel::Keep;
    double     terminal_velocity = DEFAULT_TERMINAL_VELOCITY;

    FluidModel fluid_model       = FluidModel::Keep;
    double     fluid_density     = QuadraticFluidDrag::WATER_DENSITY;
    double     drag_coefficient  = QuadraticFluidDrag::DRAG_COEFFICIENT;
    double     body_mass         = QuadraticFluidDrag::BODY_MASS;
    double     fluid_linear_drag = DEFAULT_FLUID_LINEAR;
    double     tick_damping      = TickFluidDamping::MINECRAFT_FACTOR;
    double     damping_ticks     = TickFluidDamping::MINECRAFT_TICKS;


    FlowModel  flow_model          = FlowModel::Keep;
    double     minecraft_interval  = MinecraftFluid::DEFAULT_INTERVAL;
    int        flow_spread         = MinecraftFluid::DEFAULT_SPREAD;
    int        slope_search        = MinecraftFluid::DEFAULT_SLOPE_SEARCH;
    bool       infinite_sources    = true;
    double     realistic_interval  = RealisticFluid::DEFAULT_INTERVAL;
    double     min_depth           = RealisticFluid::DEFAULT_MIN_DEPTH;

    std::shared_ptr<const AirResistance> air_resistance(double gravity, const std::shared_ptr<const AirResistance>& current) const {
        switch (air_model) {
            case AirModel::Keep:        return current;
            case AirModel::None:        return std::make_shared<NoAirResistance>();
            case AirModel::TerminalCap: return std::make_shared<TerminalVelocityCap>(terminal_velocity);
            case AirModel::WorldHeight: return std::make_shared<WorldHeightLimit>();
            case AirModel::Linear:      return std::make_shared<LinearDrag>(LinearDrag::coefficient_for(terminal_velocity, gravity));
            case AirModel::Quadratic:   return std::make_shared<QuadraticDrag>(QuadraticDrag::coefficient_for(terminal_velocity, gravity));
        }
        return current;
    }

    std::shared_ptr<const FluidResistance> fluid_resistance(const std::shared_ptr<const FluidResistance>& current) const {
        switch (fluid_model) {
            case FluidModel::Keep:        return current;
            case FluidModel::None:        return std::make_shared<NoFluidResistance>();
            case FluidModel::Linear:      return std::make_shared<LinearFluidDrag>(fluid_linear_drag);
            case FluidModel::TickDamping: return std::make_shared<TickFluidDamping>(tick_damping, damping_ticks);
            case FluidModel::Quadratic:   return std::make_shared<QuadraticFluidDrag>(fluid_density, drag_coefficient, body_mass);
        }
        return current;
    }


    std::shared_ptr<const FluidRules> fluid_rules(const std::shared_ptr<const FluidRules>& current) const {
        switch (flow_model) {
            case FlowModel::Keep:      return current;
            case FlowModel::Still:     return std::make_shared<StillFluid>();
            case FlowModel::Minecraft: return std::make_shared<MinecraftFluid>(minecraft_interval, flow_spread, slope_search, infinite_sources);
            case FlowModel::Realistic: return std::make_shared<RealisticFluid>(realistic_interval, min_depth);
        }
        return current;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_PHYSICS_SETTINGS_HPP