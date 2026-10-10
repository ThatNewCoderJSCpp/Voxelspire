#ifndef VOXELSPIRE_WORLD_CLIMATE_HPP
#define VOXELSPIRE_WORLD_CLIMATE_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include "../core/noise.hpp"
#include "biome.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

struct TerrainPoint {
    Climate climate;
    double  height = 0.0;
};

class TerrainShaper {
public:
    static constexpr double CLIMATE_SPREAD    = 1.8;
    static constexpr double ELEVATION_SCALE   = 64.0;
    static constexpr double LAPSE             = 0.3;
    static constexpr double TEMPERATURE_SCALE = 1500.0;
    static constexpr double HUMIDITY_SCALE    = 1200.0;
    static constexpr double CONTINENT_SCALE   = 1600.0;
    static constexpr double EROSION_SCALE     = 700.0;
    static constexpr double RIDGE_SCALE       = 640.0;
    static constexpr double RIVER_SCALE       = 640.0;
    static constexpr double HILL_SCALE        = 150.0;
    static constexpr double DETAIL_SCALE      = 28.0;
    static constexpr double DETAIL_HEIGHT     = 2.5;
    static constexpr double RIVER_HALF_WIDTH  = 0.045;
    static constexpr double VALLEY_HALF_WIDTH = 0.16;
    static constexpr double VALLEY_KEEP       = 0.35;
    static constexpr double VALLEY_LIFT       = 1.0;
    static constexpr double MOUNTAIN_FADE     = 0.85;
    static constexpr double FLAT_EROSION_LO   = 0.2;
    static constexpr double FLAT_EROSION_HI   = 0.8;
    static constexpr double FLAT_KEEP         = 0.45;
    static constexpr double MOUNTAIN_EROSION  = -0.05;
    static constexpr double MOUNTAIN_FULL     = -0.85;
    static constexpr double INLAND_START      = -0.2;
    static constexpr double INLAND_FULL       = 0.15;
    static constexpr double RIDGE_FLOOR       = 0.3;
    static constexpr double OCEAN_SHIFT       = 0.35;
    static constexpr double MOUNTAIN_SHIFT    = 0.5;
    static constexpr double FLAT_SHIFT        = 0.5;
    static constexpr double HILL_COAST_SHARE  = 0.5;
    static constexpr double RIVER_MIN_HEIGHT  = -1.0;
    static constexpr int    CLIMATE_OCTAVES   = 4;
    static constexpr int    CONTINENT_OCTAVES = 5;
    static constexpr int    RIVER_OCTAVES     = 3;
    static constexpr int    RIDGE_OCTAVES     = 3;
    static constexpr int    DETAIL_OCTAVES    = 3;
    static constexpr double RIVER_PERSISTENCE = 0.45;

    TerrainShaper(std::uint64_t seed, const TerrainSettings& s);

    TerrainPoint sample(double x, double y) const noexcept;

    void finish(TerrainPoint& p, double x, double y) const noexcept { rescale(p.climate, x, y, 1.0); }

    double sea_level_temperature(double x, double y) const noexcept { return clamp_unit(spread(m_temperature.at(x, y)) + m_temperature_bias); }

    void rescale(Climate& c, double x, double y, double scale) const noexcept;

private:
    static constexpr std::size_t   SPLINE_POINTS = 7;
    static constexpr double        ABYSS_DEPTH   = 1.3;
    static constexpr double        SHELF_DEPTH   = 0.35;
    static constexpr double        SHORE_HEIGHT  = 2.0;
    static constexpr double        PLAIN_HEIGHT  = 0.35;
    static constexpr double        MIN_SIZE      = 0.05;

    static constexpr std::array<double, SPLINE_POINTS> CONTINENT_STEPS{ -1.0, -0.7, -0.45, -0.3, -0.2, 0.15, 1.0 };
    static constexpr std::size_t DEEP_INDEX    = 1;
    static constexpr std::size_t SHELF_INDEX   = 2;
    static constexpr std::size_t SHORE_INDEX   = 4;
    static constexpr double      SHORE_STEP    = -0.3;
    static constexpr double      SHELF_STEP    = 0.15;
    static constexpr double      DEEPEST_SHELF = -0.85;
    static constexpr double      DEEP_GAP      = 0.25;
    static constexpr double      ABYSS_LIMIT   = -0.95;

    static constexpr std::uint64_t SIZE_SALT        = 0x5A17E5B10Cull;
    static constexpr std::uint64_t TEMPERATURE_SALT = 0x7E3D1A0011ull;
    static constexpr std::uint64_t HUMIDITY_SALT    = 0x4E3D1A0022ull;
    static constexpr std::uint64_t CONTINENT_SALT   = 0xC0371A0033ull;
    static constexpr std::uint64_t EROSION_SALT     = 0xE2051A0044ull;
    static constexpr std::uint64_t RIDGE_SALT       = 0x21D6E10055ull;
    static constexpr std::uint64_t RIVER_SALT       = 0x21BE210066ull;
    static constexpr std::uint64_t HILL_SALT        = 0x4111510077ull;
    static constexpr std::uint64_t DETAIL_SALT      = 0xDE7A110088ull;

    static double spread(double v) noexcept { return clamp_unit(v * CLIMATE_SPREAD); }
    static double clamp_unit(double v) noexcept { return vclamp(v, -1.0, 1.0); }
    static double mix(double a, double b, double t) noexcept { return a + (b - a) * t; }

    static double smoothstep(double e0, double e1, double v) noexcept {
        const double t = vclamp((v - e0) / (e1 - e0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    double continent_offset(double c) const noexcept;

    static double tangent(const std::array<double, SPLINE_POINTS>& xs, const std::array<double, SPLINE_POINTS>& ys, std::size_t i) noexcept;

    static double hermite(const std::array<double, SPLINE_POINTS>& xs, const std::array<double, SPLINE_POINTS>& ys, std::size_t i, double c) noexcept;

    static constexpr double MIN_STEP = 1e-6;

    using Steps = std::array<double, SPLINE_POINTS>;

    TerrainSettings m_settings;
    Steps           m_steps{};
    double          m_temperature_bias = 0.0;
    double          m_humidity_bias    = 0.0;
    FractalNoise    m_temperature, m_humidity, m_continent, m_erosion, m_ridges, m_river, m_hills, m_detail;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CLIMATE_HPP