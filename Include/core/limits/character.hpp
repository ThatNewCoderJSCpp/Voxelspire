#ifndef VOXELSPIRE_CORE_LIMITS_CHARACTER_HPP
#define VOXELSPIRE_CORE_LIMITS_CHARACTER_HPP

#include "bounds.hpp"

namespace voxelspire {

struct CharacterLimits {
    static constexpr Bounds width                = Bounds::at_least(0.2, 2.0);
    static constexpr Bounds reach                = Bounds::at_least(1.0, 20.0);
    static constexpr Bounds mass                 = Bounds::at_least(10.0, 300.0);
    static constexpr Bounds standing_height      = Bounds::at_least(0.2, 4.0);
    static constexpr Bounds crouching_height     = Bounds::at_least(0.2, 4.0);
    static constexpr Bounds crawling_height      = Bounds::at_least(0.1, 4.0);
    static constexpr Bounds swimming_height      = Bounds::at_least(0.1, 4.0);
    static constexpr Bounds standing_eye_height  = Bounds::at_least(0.1, 4.0);
    static constexpr Bounds crouching_eye_height = Bounds::at_least(0.1, 4.0);
    static constexpr Bounds crawling_eye_height  = Bounds::at_least(0.05, 4.0);
    static constexpr Bounds swimming_eye_height  = Bounds::at_least(0.05, 4.0);
    static constexpr Bounds walk_speed           = Bounds::at_least(0.0, 50.0);
    static constexpr Bounds sprint_speed         = Bounds::at_least(0.0, 80.0);
    static constexpr Bounds crouch_speed         = Bounds::at_least(0.0, 30.0);
    static constexpr Bounds crawl_speed          = Bounds::at_least(0.0, 20.0);
    static constexpr Bounds swim_speed           = Bounds::at_least(0.0, 30.0);
    static constexpr Bounds stroke_speed         = Bounds::at_least(0.0, 40.0);
    static constexpr Bounds fly_speed            = Bounds::at_least(0.0, 200.0);
    static constexpr Bounds alt_walk_speed       = Bounds::at_least(0.0, 50.0);
    static constexpr Bounds alt_sprint_speed     = Bounds::at_least(0.0, 80.0);
    static constexpr Bounds alt_crouch_speed     = Bounds::at_least(0.0, 30.0);
    static constexpr Bounds alt_crawl_speed      = Bounds::at_least(0.0, 20.0);
    static constexpr Bounds alt_swim_speed       = Bounds::at_least(0.0, 30.0);
    static constexpr Bounds alt_stroke_speed     = Bounds::at_least(0.0, 40.0);
    static constexpr Bounds alt_fly_speed        = Bounds::at_least(0.0, 200.0);
    static constexpr Bounds jump_velocity        = Bounds::at_least(0.0, 40.0);
    static constexpr Bounds ground_acceleration  = Bounds::at_least(1.0, 300.0);
    static constexpr Bounds air_acceleration     = Bounds::at_least(0.0, 300.0);
    static constexpr Bounds swim_acceleration    = Bounds::at_least(1.0, 300.0);
    static constexpr Bounds stroke_acceleration  = Bounds::at_least(1.0, 300.0);
    static constexpr Bounds swim_rise_speed      = Bounds::at_least(0.0, 10.0);
    static constexpr Bounds swim_sink_speed      = Bounds::at_least(0.0, 10.0);
    static constexpr Bounds swim_vertical_accel  = Bounds::at_least(0.5, 60.0);
    static constexpr Bounds surface_leap         = Bounds::at_least(0.0, 3.0);
    static constexpr Bounds stroke_buoyancy      = Bounds::at_least(0.0, 2.0);
    static constexpr Bounds fly_vertical_speed   = Bounds::at_least(0.0, 100.0);
    static constexpr Bounds fly_vertical_accel   = Bounds::at_least(1.0, 300.0);
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_LIMITS_CHARACTER_HPP