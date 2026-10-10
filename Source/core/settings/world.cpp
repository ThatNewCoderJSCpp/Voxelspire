#include "core/settings/world.hpp"

namespace voxelspire {

std::shared_ptr<const AirResistance> WorldDefaults::air_resistance() {
    return std::make_shared<QuadraticDrag>(QuadraticDrag::coefficient_for(terminal_velocity, gravity));
}

WorldSettings WorldSettings::validated() const {
    WorldSettings w = *this;
    w.min_z = vclamp(w.min_z, EngineLimits::WORLD_MIN_Z, EngineLimits::WORLD_MAX_Z - 1);
    w.max_z = vclamp(w.max_z, w.min_z + 1, EngineLimits::WORLD_MAX_Z);
    w.horizontal_limit = vclamp(w.horizontal_limit, EngineLimits::CHUNK_SIZE, EngineLimits::WORLD_MAX_HORIZONTAL);
    if (!w.air_resistance) w.air_resistance = std::make_shared<NoAirResistance>();
    if (!w.fluid_resistance) w.fluid_resistance = std::make_shared<NoFluidResistance>();
    if (!w.fluid_rules) w.fluid_rules = std::make_shared<StillFluid>();
    return w;
}

} // namespace voxelspire
