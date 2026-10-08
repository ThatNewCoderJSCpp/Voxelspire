#ifndef VOXELSPIRE_PLAYER_DEFAULTS_HPP
#define VOXELSPIRE_PLAYER_DEFAULTS_HPP

namespace voxelspire {

struct PlayerDefaults {
    static constexpr double width = 0.64;
    static constexpr double reach = 5.00;
    static constexpr double mass  = 70.0;

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
        static constexpr double lowest_health             = 0.50;
        static constexpr double weak_speed                = 0.15;
        static constexpr double thirst_interval           = 0.25;
        static constexpr double stuffed_speed             = 0.05;
        static constexpr double stamina_delay             = 0.50;
        static constexpr double exhausted_sprint          = 0.40;
        static constexpr double exhausted_rise            = 0.50;
        static constexpr double weak_regen                = 0.50;
        static constexpr double heavy_speed               = 0.25;
        static constexpr double overloaded_speed          = 0.60;
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
        static constexpr double thirst_drain      = 2.0 / 3.0;
        static constexpr double thirst_cost       = 1.0 / 3.0;
        static constexpr double cold_hunger       = 0.5;
        static constexpr double hot_thirst        = 1.0;
        static constexpr double overheat_thirst   = 2.0;
        static constexpr double walk_need         = 0.3;
        static constexpr double sprint_need       = 2.0;
        static constexpr double crouch_need       = 0.1;
        static constexpr double crawl_need        = 0.5;
        static constexpr double tread_need        = 1.0;
        static constexpr double stroke_need       = 1.5;
        static constexpr double heavy_stamina     = 1.0;
        static constexpr double heavy_hunger      = 0.5;
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

    struct health {
        static constexpr double max            = 20.0;
        static constexpr double start          = max;
        static constexpr double hearts         = 10.0;
        static constexpr double per_heart      = max / hearts;
        static constexpr double half_heart     = per_heart * (1.0 - decreases::lowest_health);
        static constexpr double lowest         = half_heart;
        static constexpr double regen_rate     = 15.0;
        static constexpr double regen_delay    = 3.0;
        static constexpr double regen_hunger   = 0.9;
        static constexpr double regen_thirst   = 0.5;
        static constexpr double regen_cost     = 1.5;
        static constexpr double safe_fall      = 3.0;
        static constexpr double fall_per_block = half_heart;
    };

    struct hunger {
        static constexpr double max        = 100.0;
        static constexpr double start      = max;
        static constexpr double drain      = 1.5;
        static constexpr double weak_below = 0.2;
        static constexpr double weak_speed = 1.0 - decreases::weak_speed;
        static constexpr double damage     = health::half_heart;
        static constexpr double interval   = 4.0;
        static constexpr double building   = 0.5;
        static constexpr double lowest     = health::lowest;
    };

    struct thirst {
        static constexpr double max      = hunger::max;
        static constexpr double start    = max;
        static constexpr double drain    = hunger::drain * (1.0 + increases::thirst_drain);
        static constexpr double damage   = hunger::damage;
        static constexpr double interval = hunger::interval * (1.0 - decreases::thirst_interval);
    };

    struct drinking {
        static constexpr double sip_share = 0.2;
        static constexpr double amount    = thirst::max * sip_share;
        static constexpr double seconds   = 0.8;
    };

    struct digestion {
        static constexpr double capacity      = hunger::max;
        static constexpr double per_minute    = 0.2;
        static constexpr double rate          = capacity * per_minute;
        static constexpr double stuffed_at    = 0.9;
        static constexpr double stuffed_speed = 1.0 - decreases::stuffed_speed;
    };

    struct stamina {
        static constexpr double max          = 100.0;
        static constexpr double start        = max;
        static constexpr double regen        = 15.0;
        static constexpr double regen_delay  = health::regen_delay * (1.0 - decreases::stamina_delay);
        static constexpr double recover_at   = 0.3;
        static constexpr double weak_regen   = 1.0 - decreases::weak_regen;
        static constexpr double hunger_cost  = 3.0;
        static constexpr double thirst_cost  = hunger_cost * (1.0 + increases::thirst_cost);
        static constexpr double sprint_speed = 1.0 - decreases::exhausted_sprint;
        static constexpr double sink         = 4.0;
        static constexpr double rise         = 1.0 - decreases::exhausted_rise;
        static constexpr double damage       = health::half_heart;
        static constexpr double interval     = 3.0;
    };

    struct breath {
        static constexpr double max      = 15.0;
        static constexpr double recover  = 5.0;
        static constexpr double damage   = health::per_heart;
        static constexpr double interval = 1.0;
    };

    struct climate {
        static constexpr double damage             = health::half_heart;
        static constexpr double interval           = hunger::interval;
        static constexpr double cold_hunger        = 1.0 + increases::cold_hunger;
        static constexpr double hot_thirst         = 1.0 + increases::hot_thirst;
        static constexpr double overheating_thirst = 1.0 + increases::overheat_thirst;
    };

    struct weight {
        static constexpr double comfortable      = mass * 2.0;
        static constexpr double max              = comfortable * 2.0;
        static constexpr double heavy_speed      = 1.0 - decreases::heavy_speed;
        static constexpr double overloaded_speed = 1.0 - decreases::overloaded_speed;
        static constexpr double stamina_extra    = increases::heavy_stamina;
        static constexpr double hunger_extra     = increases::heavy_hunger;
        static constexpr double swim_sink        = 3.0;
    };

    struct effort {
        static constexpr double resting       = 1.0;
        static constexpr double walk          = resting * (1.0 + increases::walk_need);
        static constexpr double sprint        = resting * (1.0 + increases::sprint_need);
        static constexpr double crouch        = resting * (1.0 + increases::crouch_need);
        static constexpr double crawl         = resting * (1.0 + increases::crawl_need);
        static constexpr double tread         = resting * (1.0 + increases::tread_need);
        static constexpr double stroke        = resting * (1.0 + increases::stroke_need);
        static constexpr double fly           = resting;
        static constexpr double sprint_tiring = 6.0;
        static constexpr double crawl_tiring  = 0.5;
        static constexpr double stroke_tiring = sprint_tiring;
        static constexpr double jump_tiring   = 5.0;
        static constexpr double jump_need     = 0.1;
    };
};

} // namespace voxelspire

#endif // VOXELSPIRE_PLAYER_DEFAULTS_HPP