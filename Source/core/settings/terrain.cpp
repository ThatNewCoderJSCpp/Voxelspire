#include "core/settings/terrain.hpp"

namespace voxelspire {

bool BiomeOptions::is_default() const noexcept {
    return enabled && weight == DEFAULT_WEIGHT && size == DEFAULT_SIZE && temperature == 0.0 && humidity == 0.0
        && top.empty() && filler.empty() && underwater.empty() && cliff.empty() && deep.empty() && !filler_depth && !freezes
        && !mean_temperature && !daily_swing && !season_swing && !rainfall && !waves;
}

auto TerrainSettings::biome(const std::string& id) const -> const BiomeOptions& {
    static const BiomeOptions fallback;
    auto it = biomes.find(id);
    return it == biomes.end() ? fallback : it->second;
}

} // namespace voxelspire
