#include "core/settings/physics.hpp"

namespace voxelspire {

std::shared_ptr<const AirResistance> PhysicsSettings::air_resistance(double gravity, const std::shared_ptr<const AirResistance>& current) const {
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

} // namespace voxelspire
