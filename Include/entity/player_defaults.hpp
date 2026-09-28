#ifndef VOXELSPIRE_PLAYER_DEFAULTS_HPP
#define VOXELSPIRE_PLAYER_DEFAULTS_HPP

namespace voxelspire {

struct PlayerDefaults {
    static constexpr double width              = 0.64;
    static constexpr double height_multiplier  = 3.0;
    static constexpr double eye_height_divisor = 1.5;
    static constexpr double reach              = 5.0;

    struct decreases {
        static constexpr double crouching_height          = 0.15;
        static constexpr double crawling_height           = 0.65;
        static constexpr double crouching_eye_height      = 0.30;
        static constexpr double crawling_eye_height       = 0.70;
        static constexpr double air_acceleration_from_air = 0.80;
    };

    struct height {
        static constexpr double standing  = width * height_multiplier;
        static constexpr double crouching = standing * (1.0 - decreases::crouching_height);
        static constexpr double crawling  = crouching * (1.0 - decreases::crawling_height);
    };

    struct eye_height {
        static constexpr double standing  = height::standing / eye_height_divisor;
        static constexpr double crouching = standing * (1.0 - decreases::crouching_eye_height);
        static constexpr double crawling  = crouching * (1.0 - decreases::crawling_eye_height);
    };

    struct movement {
        static constexpr double movement_speed      =  5.0;
        static constexpr double jump_velocity       = 10.0;
        static constexpr double ground_acceleration = 60.0;
        static constexpr double air_acceleration    = ground_acceleration * (1.0 - decreases::air_acceleration_from_air);
    };
};

} // namespace voxelspire

#endif // VOXELSPIRE_PLAYER_DEFAULTS_HPP