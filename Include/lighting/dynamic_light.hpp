#ifndef VOXELSPIRE_LIGHTING_DYNAMIC_LIGHT_HPP
#define VOXELSPIRE_LIGHTING_DYNAMIC_LIGHT_HPP

#include <cstdint>
#include <unordered_map>
#include "../core/limits.hpp"

namespace voxelspire {

struct DynamicLight {
    vector3d      position{};
    Color         color               = Color(255, 196, 128);
    double        intensity           = 1.0;
    double        radius              = 12.0;
    int           level               = 12;
    bool          affects_light_level = false;
    bool          casts_shadows       = false;
    double        heat                = 0.0;
    vector3d      direction           { 0.0, 0.0, -1.0 };
    double        cone                = 0.0;
    double        cone_softness       = 0.2;
    bool          moving              = true;
    std::uint64_t id                  = 0;

    static constexpr double FULL_CONE        = 180.0;
    static constexpr double TORCH_HEAT       = 15.0;
    static constexpr double FLASHLIGHT_CONE  = 22.0;
    static constexpr double FLASHLIGHT_REACH = 24.0;
    static constexpr double LAMP_CONE        = 40.0;

    bool is_spot() const noexcept { return cone > 0.0 && cone < FULL_CONE; }

    static DynamicLight torch() { DynamicLight l; l.heat = TORCH_HEAT; return l; }

    static DynamicLight glow(const Color& c, double reach, bool counts_as_light_level = false, double heat = 0.0);

    static DynamicLight spot(const vector3d& facing, const Color& c, double reach, double half_angle, bool moves, std::uint64_t key = 0);

    static DynamicLight flashlight(const vector3d& facing, std::uint64_t key = 0) {
        return spot(facing, Color(255, 236, 210), FLASHLIGHT_REACH, FLASHLIGHT_CONE, true, key);
    }

    static DynamicLight lamp(const vector3d& facing, const Color& c, double reach, std::uint64_t key = 0) {
        return spot(facing, c, reach, LAMP_CONE, false, key);
    }
};

class DynamicLights {
public:
    using Id = std::uint64_t;

    static constexpr Id REGISTRY_TAG = Id(1) << 62;

    Id add(const DynamicLight& light);

    bool remove(Id id) { return m_lights.erase(id) != 0; }

    DynamicLight* get(Id id) noexcept {
        auto it = m_lights.find(id);
        return it == m_lights.end() ? nullptr : &it->second;
    }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (const auto& kv : m_lights) fn(kv.second);
    }

    std::size_t size() const noexcept { return m_lights.size(); }
    void clear() noexcept { m_lights.clear(); }

private:
    std::unordered_map<Id, DynamicLight> m_lights;
    Id                                   m_last = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_DYNAMIC_LIGHT_HPP