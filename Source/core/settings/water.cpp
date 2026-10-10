#include "core/settings/water.hpp"

namespace voxelspire {

WaterSettings WaterSettings::still() {
    WaterSettings w;
    w.name          = STILL;
    w.flow          = FlowModel::Still;
    w.resistance    = FluidModel::Linear;
    w.current_speed = 0.0;
    w.current_push  = 0.0;
    w.wade_slowdown = STILL_WADE_SLOWDOWN;
    return w;
}

WaterSettings WaterSettings::minecraft() {
    WaterSettings w;
    w.name             = MINECRAFT;
    w.flow             = FlowModel::Minecraft;
    w.resistance       = FluidModel::TickDamping;
    w.fall_break_depth = MINECRAFT_FALL_BREAK;
    return w;
}

WaterSettings WaterSettings::ultra() {
    WaterSettings w;
    w.name               = ULTRA;
    w.realistic_interval = ULTRA_INTERVAL;
    w.min_depth          = ULTRA_MIN_DEPTH;
    w.drop_search        = RealisticFluid::MAX_DROP_SEARCH;
    w.updates            = ULTRA_UPDATES;
    w.wade_slowdown      = ULTRA_WADE;
    return w;
}

bool WaterSettings::same_flow(const WaterSettings& o) const noexcept {
    if (flow != o.flow) return false;
 
    switch (flow) {
        case FlowModel::Still:     return true;
        case FlowModel::Minecraft: return minecraft_interval == o.minecraft_interval && flow_spread == o.flow_spread && slope_search == o.slope_search && infinite_sources == o.infinite_sources;
        case FlowModel::Realistic: return realistic_interval == o.realistic_interval && min_depth == o.min_depth && seek_drops == o.seek_drops && drop_search == o.drop_search
                                       && displacement == o.displacement && splashes == o.splashes && splash == o.splash;
    }
 
    return false;
}

std::shared_ptr<const FluidRules> WaterSettings::rules(int sea_level) const {
    switch (flow) {
        case FlowModel::Still:     return std::make_shared<StillFluid>();
        case FlowModel::Minecraft: return std::make_shared<MinecraftFluid>(minecraft_interval, flow_spread, slope_search, infinite_sources);
        case FlowModel::Realistic: return std::make_shared<RealisticFluid>(realistic_interval, min_depth, seek_drops, drop_search, displacement, splashes ? splash : 0.0, sea_level);
    }
 
    return std::make_shared<StillFluid>();
}

std::shared_ptr<const FluidResistance> WaterSettings::drag(double body_mass) const {
    switch (resistance) {
        case FluidModel::None:        return std::make_shared<NoFluidResistance>();
        case FluidModel::Linear:      return std::make_shared<LinearFluidDrag>(linear_drag);
        case FluidModel::TickDamping: return std::make_shared<TickFluidDamping>(tick_damping, damping_ticks);
        case FluidModel::Quadratic:   return std::make_shared<QuadraticFluidDrag>(fluid_density, drag_coefficient, body_mass);
    }
 
    return std::make_shared<NoFluidResistance>();
}

} // namespace voxelspire
