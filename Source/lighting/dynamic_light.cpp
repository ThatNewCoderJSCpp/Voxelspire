#include "lighting/dynamic_light.hpp"

namespace voxelspire {

DynamicLight DynamicLight::glow(const Color& c, double reach, bool counts_as_light_level, double heat) {
    DynamicLight l;
    l.color  = c;
    l.radius = reach;
    l.affects_light_level = counts_as_light_level;
    l.heat   = heat;
    return l;
}

DynamicLight DynamicLight::spot(const vector3d& facing, const Color& c, double reach, double half_angle, bool moves, std::uint64_t key) {
    DynamicLight l;
    l.color     = c;
    l.radius    = reach;
    l.direction = facing;
    l.cone      = half_angle;
    l.moving    = moves;
    l.id        = key;
    return l;
}

auto DynamicLights::add(const DynamicLight& light) -> Id {
    const Id id = ++m_last;
    auto it = m_lights.emplace(id, light).first;
    if (it->second.id == 0) it->second.id = id | REGISTRY_TAG;
    return id;
}

} // namespace voxelspire
