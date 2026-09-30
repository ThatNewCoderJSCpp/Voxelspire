#ifndef VOXELSPIRE_PLAYER_DEFAULTS_HPP
#define VOXELSPIRE_PLAYER_DEFAULTS_HPP

namespace voxelspire {

struct PlayerDefaults {
    static constexpr double width = 0.64;
    static constexpr double reach = 5.0;

    struct decreases {
        static constexpr double crouching_height          = 0.15;
        static constexpr double crawling_height           = 0.65;
        static constexpr double crouching_eye_height      = 0.30;
        static constexpr double crawling_eye_height       = 0.70;
        static constexpr double swimming_height           = 0.65;
        static constexpr double swimming_eye_height       = 0.65;
        static constexpr double air_acceleration_from_air = 0.80;
        static constexpr double eye_height_divisor        = 1.50;
        static constexpr double crouch_speed              = 0.50;
        static constexpr double crawl_speed               = 0.30;
        static constexpr double swim_speed                = 0.40;
        static constexpr double swim_acceleration         = 0.50;
        static constexpr double stroke_acceleration       = 0.40;
        static constexpr double alt_walk_speed            = 0.50;
        static constexpr double alt_crouch_speed          = 0.50;
        static constexpr double alt_crawl_speed           = 0.50;
    };

    struct increases {
        static constexpr double height_multiplier = 2.0;
        static constexpr double sprint_speed      = 0.5;
        static constexpr double stroke_speed      = 0.8;
        static constexpr double fly_speed         = 1.0;
        static constexpr double alt_sprint_speed  = 0.3;
        static constexpr double alt_swim_speed    = 0.6;
        static constexpr double alt_stroke_speed  = 0.35;
        static constexpr double alt_fly_speed     = 1.5;
    };

    struct height {
        static constexpr double standing  = width     * (1.0 + increases::height_multiplier);
        static constexpr double crouching = standing  * (1.0 - decreases::crouching_height);
        static constexpr double crawling  = crouching * (1.0 - decreases::crawling_height);
        static constexpr double swimming  = crouching * (1.0 - decreases::swimming_height);
    };

    struct eye_height {
        static constexpr double standing  = height::standing / decreases::eye_height_divisor;
        static constexpr double crouching = standing  * (1.0 - decreases::crouching_eye_height);
        static constexpr double crawling  = crouching * (1.0 - decreases::crawling_eye_height);
        static constexpr double swimming  = crouching * (1.0 - decreases::swimming_eye_height);
    };

    struct movement {
        static constexpr double movement_speed      = 5.0;
        static constexpr double sprint_speed        = movement_speed * (1.0 + increases::sprint_speed);
        static constexpr double crouch_speed        = movement_speed * (1.0 - decreases::crouch_speed);
        static constexpr double crawl_speed         = movement_speed * (1.0 - decreases::crawl_speed);
        static constexpr double swim_speed          = movement_speed * (1.0 - decreases::swim_speed);
        static constexpr double stroke_speed        = swim_speed     * (1.0 + increases::stroke_speed);
        static constexpr double fly_speed           = movement_speed * (1.0 + increases::fly_speed);
        static constexpr double jump_velocity       = 10.0;
        static constexpr double ground_acceleration = 50.0;
        static constexpr double air_acceleration    = ground_acceleration * (1.0 - decreases::air_acceleration_from_air);
        static constexpr double swim_acceleration   = ground_acceleration * (1.0 - decreases::swim_acceleration);
        static constexpr double stroke_acceleration = ground_acceleration * (1.0 - decreases::stroke_acceleration);
        static constexpr double alt_walk_speed      = movement_speed      * (1.0 - decreases::alt_walk_speed);
        static constexpr double alt_sprint_speed    = sprint_speed        * (1.0 + increases::alt_sprint_speed);
        static constexpr double alt_crouch_speed    = crouch_speed        * (1.0 - decreases::alt_crouch_speed);
        static constexpr double alt_crawl_speed     = crawl_speed         * (1.0 - decreases::alt_crawl_speed);
        static constexpr double alt_swim_speed      = swim_speed          * (1.0 + increases::alt_swim_speed);
        static constexpr double alt_stroke_speed    = stroke_speed        * (1.0 + increases::alt_stroke_speed);
        static constexpr double alt_fly_speed       = fly_speed           * (1.0 + increases::alt_fly_speed);
    };

    struct swimming {
        static constexpr double rise_speed      = 2.0;
        static constexpr double sink_speed      = 0.5;
        static constexpr double vertical_accel  = 8.0;
        static constexpr double surface_leap    = 1.0;
        static constexpr double stroke_buoyancy = 0.15;
    };

    struct flying {
        static constexpr double vertical_speed = 6.0;
        static constexpr double vertical_accel = 30.0;
    };
};

} // namespace voxelspire

#endif // VOXELSPIRE_PLAYER_DEFAULTS_HPP