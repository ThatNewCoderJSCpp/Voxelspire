#include "weather/climate_field.hpp"

namespace voxelspire {

ClimateField::ClimateField(std::uint64_t seed) noexcept : m_local(seed ^ LOCAL_SALT, { 1.0, LOCAL_OCTAVES }),
          m_drift(seed ^ DRIFT_SALT, { 1.0, DRIFT_OCTAVES }) {}

double ClimateField::celsius_of(double level) noexcept {
    if (level <= MAP_LEVELS.front()) return MAP_CELSIUS.front();

    for (std::size_t i = 1; i < MAP_POINTS; ++i) {
        if (level > MAP_LEVELS[i]) continue;
        const double t = (level - MAP_LEVELS[i - 1]) / (MAP_LEVELS[i] - MAP_LEVELS[i - 1]);
        return MAP_CELSIUS[i - 1] + (MAP_CELSIUS[i] - MAP_CELSIUS[i - 1]) * t;
    }

    return MAP_CELSIUS.back();
}

BiomeClimate ClimateField::around(const WorldGenerator& generator, const TerrainSettings& terrain, const TemperatureSettings& t, const vector3d& p) const {
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

double ClimateField::nearby_change(const TemperatureSettings& t, const vector3d& p, double game_days) const noexcept {
    const double local_size = vmax(t.local_size, MIN_SIZE), drift_size = vmax(t.drift_size, MIN_SIZE);
    const double local = t.local_variation * m_local.at(p.x / local_size, p.y / local_size, game_days * LOCAL_SPEED);
    const double drift = t.drift * m_drift.at(p.x / drift_size, p.y / drift_size, game_days * t.drift_speed);
    return local + drift;
}

BiomeClimate ClimateField::at_node(const WorldGenerator& generator, const TerrainSettings& terrain, const TemperatureSettings& t, double px, double py) const {
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

} // namespace voxelspire
