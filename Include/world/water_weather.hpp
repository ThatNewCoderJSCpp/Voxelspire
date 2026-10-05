#ifndef VOXELSPIRE_WORLD_WATER_WEATHER_HPP
#define VOXELSPIRE_WORLD_WATER_WEATHER_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include "../core/random.hpp"
#include "../core/settings.hpp"
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

    static bool open_air(const World& w, const BlockPos& p) {
        return w.block_id_at(p) == AIR_ID && w.in_build_range(p) && w.column_loaded(World::column_of(p));
    }

    static bool solid(const World& w, const BlockPos& p) {
        const BlockTraits& t = w.traits_at(p);
        return t.solid && t.full_cube;
    }

    static void set_units(World& w, BlockId water, const BlockPos& p, int units) {
        if (units <= 0) w.set_block(p, AIR_ID);
        else w.set_block(p, water, RealisticFluid::state_for(units));
    }

    int add(World& w, BlockId water, const BlockPos& p, int amount) {
        if (amount <= 0) return 0;
        if (w.block_id_at(p) == water) {
            const int have = units_at(w, p);
            const int room = UNITS - have;
            const int put = vmin(room, amount);
            if (put > 0) set_units(w, water, p, have + put);
            const BlockPos up{ p.x, p.y, p.z + 1 };
            if (amount > put && open_air(w, up)) { set_units(w, water, up, amount - put); return amount; }
            return put;
        }

        if (!open_air(w, p)) return 0;
        set_units(w, water, p, vmin(amount, UNITS));
        return vmin(amount, UNITS);
    }

    bool top_of(const World& w, int x, int y, double z, BlockPos& out) const {
        if (!w.column_loaded(World::column_of(BlockPos{ x, y, 0 }))) return false;
        const int from = static_cast<int>(std::floor(z)) + SCAN_UP, to = static_cast<int>(std::floor(z)) - SCAN_DOWN;

        for (int k = from; k > to; --k) {
            if (w.block_id_at({ x, y, k }) == AIR_ID) continue;
            out = { x, y, k };
            return k < from;
        }

        return false;
    }

    BlockPos pick(const vector3d& center, double reach) {
        const double a = m_rng.range(0.0, 2.0 * PI), r = reach * std::sqrt(m_rng.unit());
        return { static_cast<int>(std::floor(center.x + std::cos(a) * r)), static_cast<int>(std::floor(center.y + std::sin(a) * r)), 0 };
    }

    void rain(double dt, const WaterWeatherContext& c) {
        const WaterSettings& s = c.settings;
        if (!s.rain_fills || c.hour_seconds <= 0.0) return;
        const bool raining = c.weather.amount > 0.0 && liquid(c.weather.precipitation);
        m_rain_budget = vmin(m_rain_budget + dt * RAIN_SAMPLES, static_cast<double>(MAX_SAMPLES_FRAME));
        const double area = PI * s.rain_reach * s.rain_reach / RAIN_SAMPLES;
        const double per_sample = area / c.hour_seconds * UNITS;
        const double fill = raining ? s.rain_fill * c.weather.amount * per_sample : 0.0;
        const double soak = s.soak * per_sample;
        const double dry  = raining ? 0.0 : s.evaporation * per_sample;
        World& w = c.world;

        for (; m_rain_budget >= 1.0; m_rain_budget -= 1.0) {
            const BlockPos col = pick(c.center, s.rain_reach);
            BlockPos top;
            if (!top_of(w, col.x, col.y, c.center.z, top)) continue;
            const bool wet = w.block_id_at(top) == c.water;
            m_fill_carry += fill;
            const int put = static_cast<int>(m_fill_carry);
            m_fill_carry -= put;

            if (!wet) {
                if (solid(w, top)) soak_ground(w, c.water, top, put, soak);
                continue;
            }

            if (put > 0) m_stats.rained += add(w, c.water, top, put);
            const int have = units_at(w, top);
            if (have >= UNITS && top.z < c.sea_level) continue;
            const bool puddle = solid(w, { top.x, top.y, top.z - 1 });
            double take = (puddle ? soak : 0.0) + (have < UNITS && top.z >= c.sea_level ? dry : 0.0);
            m_drain_carry += take;
            const int removed = vmin(static_cast<int>(m_drain_carry), have);
            m_drain_carry -= static_cast<int>(m_drain_carry);
            if (removed <= 0) continue;
            set_units(w, c.water, top, have - removed);
            m_stats.drained += removed;
        }
    }

    static std::uint64_t key_of(const BlockPos& p) noexcept {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(p.x)) << 32) | static_cast<std::uint32_t>(p.y);
    }

    void soak_ground(World& w, BlockId water, const BlockPos& top, int put, double soak) {
        const std::uint64_t key = key_of(top);
        auto it = m_ground.find(key);
        double wet = (it == m_ground.end() ? 0.0 : it->second) + put - soak;

        if (wet <= 0.0) {
            if (it != m_ground.end()) m_ground.erase(it);
            return;
        }

        if (wet >= PUDDLE_UNITS) {
            const int placed = add(w, water, { top.x, top.y, top.z + 1 }, static_cast<int>(wet));
            m_stats.rained += placed;
            wet -= placed;
        }

        if (it == m_ground.end()) {
            if (m_ground.size() >= MAX_GROUND) m_ground.clear();
            m_ground.emplace(key, static_cast<float>(wet));
        } else {
            it->second = static_cast<float>(wet);
        }
    }

    void waves(double dt, const WaterWeatherContext& c) {
        const WaterSettings& s = c.settings;
        if (!s.waves.enabled || !s.waves.overtop || c.waves.height() < MIN_WAVE) { m_wave_budget = 0.0; return; }
        m_wave_budget = vmin(m_wave_budget + dt * WAVE_SAMPLES, static_cast<double>(MAX_SAMPLES_FRAME));
        World& w = c.world;

        for (; m_wave_budget >= 1.0; m_wave_budget -= 1.0) {
            const BlockPos col = pick(c.center, WAVE_REACH);
            BlockPos top;
            if (!top_of(w, col.x, col.y, c.center.z, top) || w.block_id_at(top) != c.water) continue;
            const double scale = c.scale ? (*c.scale)(top.x, top.y).waves : 1.0;
            const double crest = top.z + w.fluid_height(top) + c.waves.at(top.x + HALF, top.y + HALF, scale);
            const double lip = top.z + 1.0;
            if (crest <= lip) continue;

            for (const auto& d : SIDES) {
                const BlockPos wall{ top.x + d[0], top.y + d[1], top.z };
                const BlockPos over{ wall.x, wall.y, wall.z + 1 };
                if (!solid(w, wall) || !open_air(w, over)) continue;
                const BlockPos inside{ wall.x + d[0], wall.y + d[1], wall.z };
                const BlockPos target = open_air(w, inside) || w.block_id_at(inside) == c.water ? inside : over;
                const int have = units_at(w, top);
                const int amount = vmin(static_cast<int>((crest - lip) * UNITS * s.waves.overtop_rate * SPILL_SHARE), have);
                if (amount <= 0) continue;
                const int moved = add(w, c.water, target, amount);
                if (moved <= 0) continue;
                set_units(w, c.water, top, have - moved);
                m_stats.spilled += moved;
                break;
            }
        }
    }

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