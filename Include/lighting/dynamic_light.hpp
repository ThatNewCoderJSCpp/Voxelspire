#ifndef VOXELSPIRE_LIGHTING_DYNAMIC_LIGHT_HPP
#define VOXELSPIRE_LIGHTING_DYNAMIC_LIGHT_HPP

#include <cstdint>
#include <unordered_map>
#include "../core/types.hpp"

namespace voxelspire {

struct DynamicLight {
    vector3d position{};
    Color    color               = Color(255, 196, 128);
    double   intensity           = 1.0;
    double   radius              = 12.0;
    int      level               = 12;
    bool     affects_light_level = false;
    bool     casts_shadows       = false;

    static DynamicLight torch() { return {}; }

    static DynamicLight glow(const Color& c, double reach, bool counts_as_light_level = false) {
        DynamicLight l;
        l.color  = c;
        l.radius = reach;
        l.affects_light_level = counts_as_light_level;
        return l;
    }
};

class DynamicLights {
public:
    using Id = std::uint64_t;

    Id add(const DynamicLight& light) {
        const Id id = ++m_last;
        m_lights.emplace(id, light);
        return id;
    }

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