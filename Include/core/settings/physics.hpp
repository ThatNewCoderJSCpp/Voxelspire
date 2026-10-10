#ifndef VOXELSPIRE_CORE_SETTINGS_PHYSICS_HPP
#define VOXELSPIRE_CORE_SETTINGS_PHYSICS_HPP

#include <cstdint>
#include <memory>
#include "../../physics/air_resistance.hpp"

namespace voxelspire {

enum class AirModel : std::uint8_t { Keep = 0, None, TerminalCap, WorldHeight, Linear, Quadratic };
enum class HeatModel : std::uint8_t { Off = 0, Simple, Realistic };

struct HeatSettings {
    HeatModel model             = HeatModel::Simple;
    bool      affects_player    = false;
    bool      effects           = false;
    double    normal_body       = 37.0;
    double    comfort_low       = 16.0;
    double    comfort_high      = 30.0;
    double    exchange_rate     = 0.02;
    double    wind_exchange     = 0.3;
    double    rain_exchange     = 0.8;
    double    water_exchange    = 3.0;
    double    ground_exchange   = 0.25;
    double    recovery_rate     = 0.6;
    double    source_strength   = 1.0;
    double    faintest_warmth   = 0.25;
    double    insulation_scale  = 1.0;
    double    cold_body         = 35.5;
    double    freezing_body     = 33.0;
    double    hot_body          = 38.5;
    double    overheating_body  = 40.0;
    double    cold_speed        = 0.9;
    double    freezing_speed    = 0.6;
    double    hot_speed         = 0.95;
    double    overheating_speed = 0.75;
    double    simple_radius     = 6.0;
    bool      simple_falloff    = true;
    double    simple_cold       = 0.0;
    double    simple_hot        = 40.0;
    double    simple_seconds    = 30.0;
};

struct PhysicsSettings {
    static constexpr double DEFAULT_TERMINAL_VELOCITY = 78.4;

    AirModel air_model         = AirModel::Keep;
    double   terminal_velocity = DEFAULT_TERMINAL_VELOCITY;
    HeatSettings heat;

    std::shared_ptr<const AirResistance> air_resistance(double gravity, const std::shared_ptr<const AirResistance>& current) const;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_PHYSICS_HPP