#ifndef VOXELSPIRE_PHYSICS_AERODYNAMICS_HPP
#define VOXELSPIRE_PHYSICS_AERODYNAMICS_HPP

#include "../core/types.hpp"
#include "../entity/player_defaults.hpp"

namespace voxelspire {

struct AerodynamicDefaults {
    static constexpr double reference_height = PlayerDefaults::height::standing;
    static constexpr double density          = 1.0;
    static constexpr double drag_coefficient = 1.0;
    static constexpr double min_height       = 0.01;
    static constexpr double min_density      = 0.01;
};

struct Aerodynamics {
    double density          = AerodynamicDefaults::density;
    double drag_coefficient = AerodynamicDefaults::drag_coefficient;

    double drag_factor(double body_height) const noexcept {
        const double h = vmax(body_height, AerodynamicDefaults::min_height);
        const double d = vmax(density, AerodynamicDefaults::min_density);
        const double reference = AerodynamicDefaults::drag_coefficient / (AerodynamicDefaults::density * AerodynamicDefaults::reference_height);
        return vmax(drag_coefficient, 0.0) / (d * h) / reference;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_AERODYNAMICS_HPP