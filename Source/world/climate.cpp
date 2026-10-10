#include "world/climate.hpp"

namespace voxelspire {

TerrainShaper::TerrainShaper(std::uint64_t seed, const TerrainSettings& s) : m_settings(s) {
    const double shelf = vmax(SHORE_STEP - SHELF_STEP * vmax(s.shelf_width, MIN_SIZE), DEEPEST_SHELF);
    m_steps = CONTINENT_STEPS;
    m_steps[SHELF_INDEX] = shelf;
    m_steps[DEEP_INDEX] = vmax(vmin(CONTINENT_STEPS[DEEP_INDEX], shelf - DEEP_GAP), ABYSS_LIMIT);
    SeededRandom rng(seed ^ SIZE_SALT);
    auto size = [&](double base) { return base * vmax(s.biome_size, MIN_SIZE) * (1.0 + rng.range(-s.size_variation, s.size_variation)); };
    auto land = [&](double base) { return base * vmax(s.continent_size, MIN_SIZE) * (1.0 + rng.range(-s.size_variation, s.size_variation)); };
    m_temperature_bias = rng.range(-s.climate_shift, s.climate_shift);
    m_humidity_bias    = rng.range(-s.climate_shift, s.climate_shift);
    m_temperature  = FractalNoise(seed ^ TEMPERATURE_SALT, { size(TEMPERATURE_SCALE), CLIMATE_OCTAVES });
    m_humidity     = FractalNoise(seed ^ HUMIDITY_SALT, { size(HUMIDITY_SCALE), CLIMATE_OCTAVES });
    m_continent    = FractalNoise(seed ^ CONTINENT_SALT, { land(CONTINENT_SCALE), CONTINENT_OCTAVES });
    m_erosion      = FractalNoise(seed ^ EROSION_SALT, { land(EROSION_SCALE), CLIMATE_OCTAVES });
    m_ridges       = FractalNoise(seed ^ RIDGE_SALT, { land(RIDGE_SCALE), RIDGE_OCTAVES });
    m_river        = FractalNoise(seed ^ RIVER_SALT, { RIVER_SCALE * vmax(s.river_width, MIN_SIZE), RIVER_OCTAVES, RIVER_PERSISTENCE });
    m_hills        = FractalNoise(seed ^ HILL_SALT, { HILL_SCALE, CLIMATE_OCTAVES });
    m_detail       = FractalNoise(seed ^ DETAIL_SALT, { DETAIL_SCALE, DETAIL_OCTAVES });
}

TerrainPoint TerrainShaper::sample(double x, double y) const noexcept {
    TerrainPoint p;
    Climate& c = p.climate;
    const TerrainSettings& s = m_settings;
    c.continentalness = clamp_unit(spread(m_continent.at(x, y)) + (1.0 - s.ocean_amount) * OCEAN_SHIFT);
    c.erosion         = spread(m_erosion.at(x, y));
    c.weirdness       = spread(m_ridges.at(x, y));
    double h = continent_offset(c.continentalness);
    const double inland = smoothstep(INLAND_START, INLAND_FULL, c.continentalness);
    const double flat_shift = (s.flatness - 1.0) * FLAT_SHIFT;
    if (h > 0.0) h *= mix(1.0, FLAT_KEEP, smoothstep(FLAT_EROSION_LO - flat_shift, FLAT_EROSION_HI - flat_shift, c.erosion));
    const double mountain_shift = (1.0 - s.mountain_amount) * MOUNTAIN_SHIFT;
    const double mountains = smoothstep(MOUNTAIN_EROSION - mountain_shift, MOUNTAIN_FULL - mountain_shift, c.erosion) * inland;
    const double ridge = 1.0 - std::fabs(c.weirdness);
    h += s.mountain_height * mountains * (RIDGE_FLOOR + (1.0 - RIDGE_FLOOR) * ridge * ridge);
    h += s.hill_height * spread(m_hills.at(x, y)) * (HILL_COAST_SHARE + (1.0 - HILL_COAST_SHARE) * inland);
    h += DETAIL_HEIGHT * s.roughness * m_detail.at(x, y);

    if (s.rivers && h > RIVER_MIN_HEIGHT) {
        const double band = std::fabs(m_river.at(x, y));
        const double fade = 1.0 - MOUNTAIN_FADE * mountains;
        const double valley = (1.0 - smoothstep(0.0, VALLEY_HALF_WIDTH, band)) * fade;
        const double bed = (1.0 - smoothstep(0.0, RIVER_HALF_WIDTH, band)) * fade;
        h = mix(h, vmin(h, VALLEY_LIFT + h * VALLEY_KEEP), valley);
        h = mix(h, -s.river_depth, bed);
        c.river = bed;
    }

    p.height = s.sea_level + h;
    c.elevation = h / ELEVATION_SCALE;
    return p;
}

void TerrainShaper::rescale(Climate& c, double x, double y, double scale) const noexcept {
    const double sx = x / vmax(scale, MIN_SIZE), sy = y / vmax(scale, MIN_SIZE);
    c.temperature = clamp_unit(spread(m_temperature.at(sx, sy)) + m_temperature_bias) - vmax(c.elevation, 0.0) * LAPSE / vmax(m_settings.snow_line, MIN_SIZE);
    c.humidity    = clamp_unit(spread(m_humidity.at(sx, sy)) + m_humidity_bias);
}

double TerrainShaper::continent_offset(double c) const noexcept {
    const double deep = m_settings.ocean_depth, land = m_settings.land_height;
    const std::array<double, SPLINE_POINTS> ys{
        -deep * ABYSS_DEPTH, -deep, -deep * SHELF_DEPTH, -SHORE_HEIGHT, SHORE_HEIGHT, land * PLAIN_HEIGHT, land
    };

    const Steps& xs = m_steps;
    if (c <= xs[0]) return ys[0];

    for (std::size_t i = 1; i < SPLINE_POINTS; ++i) {
        if (c > xs[i]) continue;
        if (i > SHORE_INDEX) return mix(ys[i - 1], ys[i], smoothstep(xs[i - 1], xs[i], c));
        return hermite(xs, ys, i - 1, c);
    }

    return ys[SPLINE_POINTS - 1];
}

double TerrainShaper::tangent(const std::array<double, SPLINE_POINTS>& xs, const std::array<double, SPLINE_POINTS>& ys, std::size_t i) noexcept {
    auto secant = [&](std::size_t a) { return (ys[a + 1] - ys[a]) / vmax(xs[a + 1] - xs[a], MIN_STEP); };
    if (i == 0) return secant(0);
    if (i >= SHORE_INDEX) return 0.0;
    const double a = secant(i - 1), b = secant(i);
    if (a * b <= 0.0) return 0.0;
    return 2.0 / (1.0 / a + 1.0 / b);
}

double TerrainShaper::hermite(const std::array<double, SPLINE_POINTS>& xs, const std::array<double, SPLINE_POINTS>& ys, std::size_t i, double c) noexcept {
    const double h = vmax(xs[i + 1] - xs[i], MIN_STEP);
    const double t = vclamp((c - xs[i]) / h, 0.0, 1.0), t2 = t * t, t3 = t2 * t;
    return (2.0 * t3 - 3.0 * t2 + 1.0) * ys[i] + (t3 - 2.0 * t2 + t) * h * tangent(xs, ys, i)
         + (-2.0 * t3 + 3.0 * t2) * ys[i + 1] + (t3 - t2) * h * tangent(xs, ys, i + 1);
}

} // namespace voxelspire
