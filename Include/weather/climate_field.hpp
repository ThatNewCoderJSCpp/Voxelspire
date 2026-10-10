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
;

    static double celsius_of(double level) noexcept;

    BiomeClimate around(const WorldGenerator& generator, const TerrainSettings& terrain, const TemperatureSettings& t, const vector3d& p) const;

    double nearby_change(const TemperatureSettings& t, const vector3d& p, double game_days) const noexcept;

private:
    struct Sample { double x, y, weight; };

    BiomeClimate at_node(const WorldGenerator& generator, const TerrainSettings& terrain, const TemperatureSettings& t, double px, double py) const;

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