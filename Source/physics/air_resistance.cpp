#include "physics/air_resistance.hpp"

namespace voxelspire {

double QuadraticDrag::coefficient_for(double terminal_velocity, double gravity) noexcept {
    return terminal_velocity > 0.0 ? std::fabs(gravity) / (terminal_velocity * terminal_velocity) : 0.0;
}

} // namespace voxelspire
