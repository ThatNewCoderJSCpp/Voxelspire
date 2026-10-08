#ifndef VOXELSPIRE_CORE_SETTINGS_TERRAIN_HPP
#define VOXELSPIRE_CORE_SETTINGS_TERRAIN_HPP

#include <optional>
#include <string>

namespace voxelspire {

struct BiomeOptions {
    static constexpr double DEFAULT_WEIGHT = 1.0;
    static constexpr double DEFAULT_SIZE   = 1.0;

    bool                  enabled     = true;
    double                weight      = DEFAULT_WEIGHT;
    double                size        = DEFAULT_SIZE;
    double                temperature = 0.0;
    double                humidity    = 0.0;
    std::string           top;
    std::string           filler;
    std::string           underwater;
    std::string           cliff;
    std::string           deep;
    std::optional<int>    filler_depth;
    std::optional<bool>   freezes;
    std::optional<double> mean_temperature;
    std::optional<double> daily_swing;
    std::optional<double> season_swing;
    std::optional<double> rainfall;
    std::optional<double> waves;

    bool is_default() const noexcept {
        return enabled && weight == DEFAULT_WEIGHT && size == DEFAULT_SIZE && temperature == 0.0 && humidity == 0.0
            && top.empty() && filler.empty() && underwater.empty() && cliff.empty() && deep.empty() && !filler_depth && !freezes
            && !mean_temperature && !daily_swing && !season_swing && !rainfall && !waves;
    }
};

struct CaveSettings {
    bool   enabled        = true;
    double tunnels        = 1.0;
    double tunnel_width   = 1.0;
    double tunnel_length  = 1.0;
    double flatness       = 1.0;
    double caverns        = 1.0;
    double cavern_size    = 1.0;
    int    cavern_roof    = 6;
    int    min_depth      = 0;
    int    max_depth      = 0;
    double deep_growth    = 0.5;
    double entrances      = 1.0;
    int    entrance_depth = 32;
    double entrance_width = 1.0;
    double canyons        = 0.6;
    int    canyon_depth   = 24;
    double canyon_width   = 1.0;
    bool   underwater     = true;
};

struct TerrainSettings {
    int          sea_level       = 64;
    int          bedrock_layers  = 3;
    double       biome_size      = 1.0;
    double       continent_size  = 0.7;
    double       size_variation  = 0.25;
    double       climate_shift   = 0.15;
    double       ocean_amount    = 0.8;
    double       land_height     = 30.0;
    double       ocean_depth     = 26.0;
    double       mountain_amount = 0.6;
    double       mountain_height = 100.0;
    double       flatness        = 1.2;
    double       hill_height     = 10.0;
    double       roughness       = 1.0;
    bool         rivers          = true;
    double       river_width     = 1.0;
    double       river_depth     = 5.0;
    double       snow_line       = 1.25;
    double       shelf_width     = 1.5;
    CaveSettings caves;
 
    std::map<std::string, BiomeOptions> biomes;
 
    const BiomeOptions& biome(const std::string& id) const {
        static const BiomeOptions fallback;
        auto it = biomes.find(id);
        return it == biomes.end() ? fallback : it->second;
    }
 
    BiomeOptions& edit_biome(const std::string& id) { return biomes[id]; }
 
    void tidy_biome(const std::string& id) {
        auto it = biomes.find(id);
        if (it != biomes.end() && it->second.is_default()) biomes.erase(it);
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_TERRAIN_HPP