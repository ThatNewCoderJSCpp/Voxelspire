#include "physics/aerodynamics.hpp"

namespace voxelspire {

double Aerodynamics::drag_factor(double body_height) const noexcept {
    const double h = vmax(body_height, AerodynamicDefaults::min_height);
    const double d = vmax(density, AerodynamicDefaults::min_density);
    const double reference = AerodynamicDefaults::drag_coefficient / (AerodynamicDefaults::density * AerodynamicDefaults::reference_height);
    return vmax(drag_coefficient, 0.0) / (d * h) / reference;
}

} // namespace voxelspire
