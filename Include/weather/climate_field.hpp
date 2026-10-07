#ifndef VOXELSPIRE_WEATHER_CLIMATE_FIELD_HPP
#define VOXELSPIRE_WEATHER_CLIMATE_FIELD_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include "../core/noise.hpp"
#include "../world/world_generator.hpp"
#include "system.hpp"

namespace voxelspire {

class ClimateField {
public:
    static constexpr std::size_t MAP_POINTS = 7;

    static constexpr std::array<double, MAP_POINTS> MAP_LEVELS{ -1.0, Biomes::FREEZING, Biomes::COOL, 0.0, Biomes::WARM, Biomes::HOT_OCEAN, 1.0 };
    static constexpr std::array<double, MAP_POINTS> MAP_CELSIUS{ -24.0, -6.0, 4.0, 12.0, 23.0, 28.0, 33.0 };

    explicit ClimateField(std::uint64_t seed = 0) noexcept
        : m_local(seed ^ LOCAL_SALT, { 1.0, LOCAL_OCTAVES }),
          m_drift(seed ^ DRIFT_SALT, { 1.0, DRIFT_OCTAVES }) {}

    static double celsius_of(double level) noexcept {
        if (level <= MAP_LEVELS.front()) return MAP_CELSIUS.front();

        for (std::size_t i = 1; i < MAP_POINTS; ++i) {
            if (level > MAP_LEVELS[i]) continue;
            const double t = (level - MAP_LEVELS[i - 1]) / (MAP_LEVELS[i] - MAP_LEVELS[i - 1]);
            return MAP_CELSIUS[i - 1] + (MAP_CELSIUS[i] - MAP_CELSIUS[i - 1]) * t;
        }

        return MAP_CELSIUS.back();
    }

    BiomeClimate around(const WorldGenerator& generator, const TerrainSettings& terrain, const TemperatureSettings& t, const vector3d& p) const {
        const double gx = p.x / GRID, gy = p.y / GRID;
        const double fx = std::floor(gx), fy = std::floor(gy);
        const double tx = gx - fx, ty = gy - fy;
        std::array<BiomeClimate, CORNERS> corners{};
        const std::array<double, CORNERS> weights{ (1.0 - tx) * (1.0 - ty), tx * (1.0 - ty), (1.0 - tx) * ty, tx * ty };

        for (std::size_t k = 0; k < CORNERS; ++k) {
            const double cx = (fx + static_cast<double>(k % 2)) * GRID, cy = (fy + static_cast<double>(k / 2)) * GRID;
            corners[k] = at_node(generator, terrain, t, cx, cy);
        }

        return BiomeWeather::blend(corners.data(), weights.data(), CORNERS);
    }

    double nearby_change(const TemperatureSettings& t, const vector3d& p, double game_days) const noexcept {
        const double local_size = vmax(t.local_size, MIN_SIZE), drift_size = vmax(t.drift_size, MIN_SIZE);
        const double local = t.local_variation * m_local.at(p.x / local_size, p.y / local_size, game_days * LOCAL_SPEED);
        const double drift = t.drift * m_drift.at(p.x / drift_size, p.y / drift_size, game_days * t.drift_speed);
        return local + drift;
    }

private:
    struct Sample { double x, y, weight; };

    BiomeClimate at_node(const WorldGenerator& generator, const TerrainSettings& terrain, const TemperatureSettings& t, double px, double py) const {
        const vector3d p{ px, py, 0.0 };
        std::array<BiomeClimate, SAMPLES> found{};
        std::array<double, SAMPLES> weights{};
        std::size_t count = 0;
        const double reach = vmax(t.blend_distance, 0.0);

        for (const Sample& s : RING) {
            const int x = static_cast<int>(std::floor(p.x + s.x * reach)), y = static_cast<int>(std::floor(p.y + s.y * reach));
            const Biome* biome = generator.biome_at(x, y);
            if (!biome) continue;
            found[count]   = BiomeWeather::effective(*biome, terrain.biome(biome->id().str()));
            weights[count] = s.weight;
            ++count;
            if (reach <= 0.0) break;
        }

        BiomeClimate c = BiomeWeather::blend(found.data(), weights.data(), count);
        const BiomeClimate even = BiomeWeather::blend(found.data(), count);
        c.rainfall = even.rainfall;
        c.waves    = even.waves;
        const std::optional<double> level = generator.climate_temperature(static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y)));
        const double mix = vclamp(t.climate_mix, 0.0, 1.0);
        if (level && count > 0) c.temperature += (celsius_of(*level) - c.temperature) * mix;
        return c;
    }

    static constexpr std::size_t   SAMPLES       = 19;
    static constexpr std::size_t   CORNERS       = 4;
    static constexpr double        GRID          = 16.0;
    static constexpr double        INNER         = 0.5;
    static constexpr double        CENTER_WEIGHT = 3.0;
    static constexpr double        INNER_WEIGHT  = 0.5;
    static constexpr double        OUTER_WEIGHT  = 0.1;
    static constexpr double        COS_30        = 0.8660254037844386;
    static constexpr double        COS_60        = 0.5;
    static constexpr double        SIN_15        = 0.25881904510252074;
    static constexpr double        COS_15        = 0.9659258262890683;
    static constexpr double        SIN_45        = 0.7071067811865476;
    static constexpr double        MIN_SIZE      = 1e-3;
    static constexpr double        LOCAL_SPEED   = 3.0;
    static constexpr int           LOCAL_OCTAVES = 2;
    static constexpr int           DRIFT_OCTAVES = 2;
    static constexpr std::uint64_t LOCAL_SALT    = 0x7E3A11C5ull;
    static constexpr std::uint64_t DRIFT_SALT    = 0x7E3A11C6ull;

    static constexpr std::array<Sample, SAMPLES> RING{ {
        {  0.0,             0.0,             CENTER_WEIGHT },
        {  INNER,           0.0,             INNER_WEIGHT  },
        {  INNER * COS_60,  INNER * COS_30,  INNER_WEIGHT  },
        { -INNER * COS_60,  INNER * COS_30,  INNER_WEIGHT  },
        { -INNER,           0.0,             INNER_WEIGHT  },
        { -INNER * COS_60, -INNER * COS_30,  INNER_WEIGHT  },
        {  INNER * COS_60, -INNER * COS_30,  INNER_WEIGHT  },
        {  COS_15,          SIN_15,          OUTER_WEIGHT  },
        {  SIN_45,          SIN_45,          OUTER_WEIGHT  },
        {  SIN_15,          COS_15,          OUTER_WEIGHT  },
        { -SIN_15,          COS_15,          OUTER_WEIGHT  },
        { -SIN_45,          SIN_45,          OUTER_WEIGHT  },
        { -COS_15,          SIN_15,          OUTER_WEIGHT  },
        { -COS_15,         -SIN_15,          OUTER_WEIGHT  },
        { -SIN_45,         -SIN_45,          OUTER_WEIGHT  },
        { -SIN_15,         -COS_15,          OUTER_WEIGHT  },
        {  SIN_15,         -COS_15,          OUTER_WEIGHT  },
        {  SIN_45,         -SIN_45,          OUTER_WEIGHT  },
        {  COS_15,         -SIN_15,          OUTER_WEIGHT }
    } };

    FractalNoise m_local;
    FractalNoise m_drift;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_CLIMATE_FIELD_HPP