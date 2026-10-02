#ifndef VOXELSPIRE_PHYSICS_PHYSICS_SETTINGS_HPP
#define VOXELSPIRE_PHYSICS_PHYSICS_SETTINGS_HPP

#include <memory>
#include "air_resistance.hpp"
#include "../core/limits.hpp"
#include "water_settings.hpp"

namespace voxelspire {

enum class AirModel : std::uint8_t { Keep = 0, None, TerminalCap, WorldHeight, Linear, Quadratic };

struct PhysicsLimits {
    static constexpr Bounds terminal_velocity { 5.0, 400.0 };
};

struct PhysicsSettings {
    static constexpr double DEFAULT_TERMINAL_VELOCITY = 78.4;

    AirModel air_model         = AirModel::Keep;
    double   terminal_velocity = DEFAULT_TERMINAL_VELOCITY;

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
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_PHYSICS_SETTINGS_HPP