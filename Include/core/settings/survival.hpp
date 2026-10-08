#ifndef VOXELSPIRE_CORE_SETTINGS_SURVIVAL_HPP
#define VOXELSPIRE_CORE_SETTINGS_SURVIVAL_HPP

#include <cstdint>
#include "../../entity/player_defaults.hpp"

namespace voxelspire {

enum class LossStyle       : std::uint8_t { Steady = 0, Building };
enum class ExhaustedSprint : std::uint8_t { Normal = 0, Slower, Blocked };

struct HealthSettings {
    bool   enabled        = true;
    double max            = PlayerDefaults::health::max;
    double start          = PlayerDefaults::health::start;
    double per_heart      = PlayerDefaults::health::per_heart;
    double damage_scale   = 1.0;
    bool   can_die        = true;
    double lowest         = PlayerDefaults::health::lowest;
    bool   regenerates    = true;
    double regen_rate     = PlayerDefaults::health::regen_rate;
    double regen_delay    = PlayerDefaults::health::regen_delay;
    double regen_hunger   = PlayerDefaults::health::regen_hunger;
    double regen_thirst   = PlayerDefaults::health::regen_thirst;
    double regen_cost     = PlayerDefaults::health::regen_cost;
    bool   fall_damage    = true;
    double safe_fall      = PlayerDefaults::health::safe_fall;
    double fall_per_block = PlayerDefaults::health::fall_per_block;
    bool   void_kills     = true;
};

struct NeedSettings {
    bool      enabled        = true;
    double    max            = PlayerDefaults::hunger::max;
    double    start          = PlayerDefaults::hunger::start;
    double    drain          = PlayerDefaults::hunger::drain;
    double    weak_below     = PlayerDefaults::hunger::weak_below;
    double    weak_speed     = PlayerDefaults::hunger::weak_speed;
    bool      weak_no_sprint = true;
    bool      hurts          = true;
    LossStyle style          = LossStyle::Steady;
    double    damage         = PlayerDefaults::hunger::damage;
    double    interval       = PlayerDefaults::hunger::interval;
    double    building       = PlayerDefaults::hunger::building;
    bool      can_kill       = false;
    double    lowest         = PlayerDefaults::hunger::lowest;

    static NeedSettings hunger() { return {}; }

    static NeedSettings thirst() {
        NeedSettings s;
        s.enabled  = false;
        s.max      = PlayerDefaults::thirst::max;
        s.start    = PlayerDefaults::thirst::start;
        s.drain    = PlayerDefaults::thirst::drain;
        s.damage   = PlayerDefaults::thirst::damage;
        s.interval = PlayerDefaults::thirst::interval;
        return s;
    }
};

struct DrinkSettings {
    bool   from_water  = true;
    double amount      = PlayerDefaults::drinking::amount;
    double seconds     = PlayerDefaults::drinking::seconds;
    bool   takes_water = false;
};

struct DigestionSettings {
    bool   enabled       = true;
    double capacity      = PlayerDefaults::digestion::capacity;
    double rate          = PlayerDefaults::digestion::rate;
    double stuffed_at    = PlayerDefaults::digestion::stuffed_at;
    double stuffed_speed = PlayerDefaults::digestion::stuffed_speed;
};

struct StaminaSettings {
    bool            enabled      = false;
    double          max          = PlayerDefaults::stamina::max;
    double          start        = PlayerDefaults::stamina::start;
    double          regen        = PlayerDefaults::stamina::regen;
    double          regen_delay  = PlayerDefaults::stamina::regen_delay;
    double          recover_at   = PlayerDefaults::stamina::recover_at;
    double          weak_regen   = PlayerDefaults::stamina::weak_regen;
    double          hunger_cost  = PlayerDefaults::stamina::hunger_cost;
    double          thirst_cost  = PlayerDefaults::stamina::thirst_cost;
    ExhaustedSprint sprint       = ExhaustedSprint::Blocked;
    double          sprint_speed = PlayerDefaults::stamina::sprint_speed;
    bool            no_jump      = false;
    double          sink         = PlayerDefaults::stamina::sink;
    double          rise         = PlayerDefaults::stamina::rise;
    bool            hurts        = true;
    double          damage       = PlayerDefaults::stamina::damage;
    double          interval     = PlayerDefaults::stamina::interval;
};

struct BreathSettings {
    bool   enabled  = true;
    double max      = PlayerDefaults::breath::max;
    double recover  = PlayerDefaults::breath::recover;
    double damage   = PlayerDefaults::breath::damage;
    double interval = PlayerDefaults::breath::interval;
};

struct ClimateHarmSettings {
    bool   freezing_hurts       = true;
    double freezing_damage      = PlayerDefaults::climate::damage;
    double freezing_interval    = PlayerDefaults::climate::interval;
    bool   overheating_hurts    = true;
    double overheating_damage   = PlayerDefaults::climate::damage;
    double overheating_interval = PlayerDefaults::climate::interval;
    double cold_hunger          = PlayerDefaults::climate::cold_hunger;
    double hot_thirst           = PlayerDefaults::climate::hot_thirst;
    double overheating_thirst   = PlayerDefaults::climate::overheating_thirst;
};

struct WeightSettings {
    bool   enabled              = false;
    double carried              = 0.0;
    double comfortable          = PlayerDefaults::weight::comfortable;
    double max                  = PlayerDefaults::weight::max;
    double heavy_speed          = PlayerDefaults::weight::heavy_speed;
    double overloaded_speed     = PlayerDefaults::weight::overloaded_speed;
    bool   overloaded_no_jump   = true;
    bool   overloaded_no_sprint = true;
    double stamina_extra        = PlayerDefaults::weight::stamina_extra;
    double hunger_extra         = PlayerDefaults::weight::hunger_extra;
    double swim_sink            = PlayerDefaults::weight::swim_sink;
};

struct ActivityCost {
    double stamina = 0.0;
    double hunger  = PlayerDefaults::effort::resting;
    double thirst  = PlayerDefaults::effort::resting;
};

struct ExertionSettings {
    using E = PlayerDefaults::effort;

    ActivityCost idle         { 0.0,              E::resting, E::resting };
    ActivityCost walk         { 0.0,              E::walk,    E::walk    };
    ActivityCost sprint       { E::sprint_tiring, E::sprint,  E::sprint  };
    ActivityCost crouch       { 0.0,              E::crouch,  E::crouch  };
    ActivityCost crawl        { E::crawl_tiring,  E::crawl,   E::crawl   };
    ActivityCost tread        { 0.0,              E::tread,   E::tread   };
    ActivityCost stroke       { E::stroke_tiring, E::stroke,  E::stroke  };
    ActivityCost fly          { 0.0,              E::fly,     E::fly     };
    double       jump_stamina = E::jump_tiring;
    double       jump_hunger  = E::jump_need;
    double       jump_thirst  = E::jump_need;
};

struct SurvivalSettings {
    bool                enabled = true;
    HealthSettings      health;
    NeedSettings        hunger  = NeedSettings::hunger();
    NeedSettings        thirst  = NeedSettings::thirst();
    DrinkSettings       drink;
    DigestionSettings   digestion;
    StaminaSettings     stamina;
    BreathSettings      breath;
    ClimateHarmSettings climate;
    WeightSettings      weight;
    ExertionSettings    effort;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_SURVIVAL_HPP