#ifndef VOXELSPIRE_CORE_SETTINGS_CHARACTER_HPP
#define VOXELSPIRE_CORE_SETTINGS_CHARACTER_HPP

#include "../../entity/entity_body.hpp"

namespace voxelspire {

struct CharacterSettings {
    double width = PlayerDefaults::width;
    double reach = PlayerDefaults::reach;
    double mass  = PlayerDefaults::mass;

    double standing_height  = PlayerDefaults::height::standing;
    double crouching_height = PlayerDefaults::height::crouching;
    double crawling_height  = PlayerDefaults::height::crawling;
    double swimming_height  = PlayerDefaults::height::swimming;

    double standing_eye_height  = PlayerDefaults::eye_height::standing;
    double crouching_eye_height = PlayerDefaults::eye_height::crouching;
    double crawling_eye_height  = PlayerDefaults::eye_height::crawling;
    double swimming_eye_height  = PlayerDefaults::eye_height::swimming;

    double walk_speed   = PlayerDefaults::movement::movement_speed;
    double sprint_speed = PlayerDefaults::movement::sprint_speed;
    double crouch_speed = PlayerDefaults::movement::crouch_speed;
    double crawl_speed  = PlayerDefaults::movement::crawl_speed;
    double swim_speed   = PlayerDefaults::movement::swim_speed;
    double stroke_speed = PlayerDefaults::movement::stroke_speed;
    double fly_speed    = PlayerDefaults::movement::fly_speed;

    double alt_walk_speed   = PlayerDefaults::movement::alt_walk_speed;
    double alt_sprint_speed = PlayerDefaults::movement::alt_sprint_speed;
    double alt_crouch_speed = PlayerDefaults::movement::alt_crouch_speed;
    double alt_crawl_speed  = PlayerDefaults::movement::alt_crawl_speed;
    double alt_swim_speed   = PlayerDefaults::movement::alt_swim_speed;
    double alt_stroke_speed = PlayerDefaults::movement::alt_stroke_speed;
    double alt_fly_speed    = PlayerDefaults::movement::alt_fly_speed;

    double jump_velocity       = PlayerDefaults::movement::jump_velocity;
    double ground_acceleration = PlayerDefaults::movement::ground_acceleration;
    double air_acceleration    = PlayerDefaults::movement::air_acceleration;
    double swim_acceleration   = PlayerDefaults::movement::swim_acceleration;
    double stroke_acceleration = PlayerDefaults::movement::stroke_acceleration;

    double swim_rise_speed     = PlayerDefaults::swimming::rise_speed;
    double swim_sink_speed     = PlayerDefaults::swimming::sink_speed;
    double swim_vertical_accel = PlayerDefaults::swimming::vertical_accel;
    double surface_leap        = PlayerDefaults::swimming::surface_leap;
    double stroke_buoyancy     = PlayerDefaults::swimming::stroke_buoyancy;

    double fly_vertical_speed = PlayerDefaults::flying::vertical_speed;
    double fly_vertical_accel = PlayerDefaults::flying::vertical_accel;

    double speed_ratio(double speed) const noexcept { return walk_speed > 0.0 ? speed / walk_speed : 0.0; }
    double acceleration_ratio(double accel) const noexcept { return ground_acceleration > 0.0 ? accel / ground_acceleration : 0.0; }

    EntityBody body() const;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_CHARACTER_HPP