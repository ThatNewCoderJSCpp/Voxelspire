#ifndef VOXELSPIRE_WORLD_WATER_WEATHER_HPP
#define VOXELSPIRE_WORLD_WATER_WEATHER_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include "../core/random.hpp"
#include "../core/settings.hpp"
#include "../physics/waves.hpp"
#include "../weather/system.hpp"
#include "world.hpp"

namespace voxelspire {

struct WaterWeatherStats {
    double rained   = 0.0;
    double drained  = 0.0;
    double spilled  = 0.0;
};

struct WaterWeatherContext {
    World&               world;
    BlockId              water;
    const WaterSettings& settings;
    const WaveField&     waves;
    const WaterLook*     scale;
    const LocalWeather&  weather;
    vector3d             center;
    int                  sea_level;
    double               hour_seconds;
};

class WaterWeather {
public:
    static constexpr int    UNITS             = RealisticFluid::UNITS;
    static constexpr double RAIN_SAMPLES      = 400.0;
    static constexpr double WAVE_SAMPLES      = 600.0;
    static constexpr double WAVE_REACH        = 24.0;
    static constexpr double MIN_WAVE          = 0.08;
    static constexpr double SPILL_SHARE       = 0.5;
    static constexpr int    SCAN_UP           = 48;
    static constexpr int    SCAN_DOWN         = 48;
    static constexpr int    MAX_SAMPLES_FRAME = 4000;

    const WaterWeatherStats& stats() const noexcept { return m_stats; }

    void update(double dt, const WaterWeatherContext& c) {
        if (dt <= 0.0 || c.settings.flow != FlowModel::Realistic) return;
        rain(dt, c);
        waves(dt, c);
    }

private:
    static constexpr std::array<std::array<int, 2>, 4> SIDES{ { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } } };

    static bool liquid(Precipitation p) noexcept { return p == Precipitation::Rain || p == Precipitation::Sleet || p == Precipitation::FreezingRain; }

    static int units_at(const World& w, const BlockPos& p) { return RealisticFluid::units(w.fluid_state(p)); }

    static bool open_air(const World& w, const BlockPos& p);

    static bool solid(const World& w, const BlockPos& p) {
        const BlockTraits& t = w.traits_at(p);
        return t.solid && t.full_cube;
    }

    static void set_units(World& w, BlockId water, const BlockPos& p, int units);

    int add(World& w, BlockId water, const BlockPos& p, int amount);

    bool top_of(const World& w, int x, int y, double z, BlockPos& out) const;

    BlockPos pick(const vector3d& center, double reach);

    void rain(double dt, const WaterWeatherContext& c);

    static std::uint64_t key_of(const BlockPos& p) noexcept;

    void soak_ground(World& w, BlockId water, const BlockPos& top, int put, double soak);

    void waves(double dt, const WaterWeatherContext& c);

    static constexpr double      HALF         = 0.5;
    static constexpr double      PUDDLE_UNITS = 0.25 * UNITS;
    static constexpr std::size_t MAX_GROUND   = 65536;

    std::unordered_map<std::uint64_t, float> m_ground;

    SeededRandom      m_rng;
    WaterWeatherStats m_stats;
    double            m_rain_budget = 0.0;
    double            m_wave_budget = 0.0;
    double            m_fill_carry  = 0.0;
    double            m_drain_carry = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_WATER_WEATHER_HPP