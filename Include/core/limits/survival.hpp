#ifndef VOXELSPIRE_CORE_LIMITS_SURVIVAL_HPP
#define VOXELSPIRE_CORE_LIMITS_SURVIVAL_HPP

#include "bounds.hpp"

namespace voxelspire {

struct SurvivalLimits {
    static constexpr Bounds health      = Bounds::at_least(1.0, 100.0);
    static constexpr Bounds per_heart   = Bounds::at_least(0.5, 20.0);
    static constexpr Bounds scale       = Bounds::at_least(0.0, 5.0);
    static constexpr Bounds lowest      = Bounds::at_least(0.1, 20.0);
    static constexpr Bounds regen_rate  = Bounds::at_least(0.0, 120.0);
    static constexpr Bounds delay       = Bounds::at_least(0.0, 60.0);
    static constexpr Bounds fraction    = { 0.0, 1.0 };
    static constexpr Bounds cost        = Bounds::at_least(0.0, 10.0);
    static constexpr Bounds fall        = Bounds::at_least(0.0, 32.0);
    static constexpr Bounds per_block   = Bounds::at_least(0.0, 10.0);
    static constexpr Bounds meter       = Bounds::at_least(1.0, 1000.0);
    static constexpr Bounds drain       = Bounds::at_least(0.0, 20.0);
    static constexpr Bounds speed       = { 0.05, 1.0 };
    static constexpr Bounds damage      = Bounds::at_least(0.0, 50.0);
    static constexpr Bounds interval    = Bounds::at_least(0.1, 60.0);
    static constexpr Bounds building    = Bounds::at_least(0.0, 5.0);
    static constexpr Bounds drink       = Bounds::at_least(0.0, 100.0);
    static constexpr Bounds drink_time  = Bounds::at_least(0.0, 5.0);
    static constexpr Bounds digest_rate = Bounds::at_least(0.1, 200.0);
    static constexpr Bounds regen       = Bounds::at_least(0.0, 100.0);
    static constexpr Bounds sink        = Bounds::at_least(0.0, 20.0);
    static constexpr Bounds breath      = Bounds::at_least(1.0, 120.0);
    static constexpr Bounds recover     = Bounds::at_least(0.1, 20.0);
    static constexpr Bounds multiplier  = Bounds::at_least(0.0, 5.0);
    static constexpr Bounds kilograms   = Bounds::at_least(0.0, 200.0);
    static constexpr Bounds stamina_use = Bounds::at_least(0.0, 50.0);
    static constexpr Bounds jump_use    = Bounds::at_least(0.0, 30.0);
    static constexpr Bounds jump_need   = Bounds::at_least(0.0, 2.0);
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_SURVIVAL_HPP