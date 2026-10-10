#ifndef VOXELSPIRE_LIGHTING_LIGHTING_DIRECTOR_HPP
#define VOXELSPIRE_LIGHTING_LIGHTING_DIRECTOR_HPP

#include <algorithm>
#include <cmath>
#include <vector>
#include "../world/world.hpp"
#include "dynamic_light.hpp"
#include "../core/settings.hpp"
#include "sun_path.hpp"

namespace voxelspire {

class LightingDirector {
public:
    void configure(const LightingSettings& settings) { m_settings = settings; }
    void set_swell(const fizmo::graphics::Swell3D& swell) noexcept { m_swell = swell; }
    const LightingSettings& settings() const noexcept { return m_settings; }

    bool enabled() const noexcept;

    bool shadows() const noexcept;

    double shadow_distance() const noexcept;

    const fizmo::graphics::SceneLighting3D& build(
        const SkyState& sky, 
        const World& world, 
        const vector3d& camera, 
        const std::vector<DynamicLight>& lights,
        double seconds = 0.0, 
        const Color* camera_medium = nullptr
    );

    LightLevel light_level_at(
        const World& world,
        const vector3d& p, 
        const std::vector<DynamicLight>& lights
    ) const;

    fizmo::graphics::BakedLight render_light_at(const World& world, const vector3d& p) const;

    const fizmo::graphics::SceneLighting3D& scene() const noexcept { return m_scene; }

    void set_reflection_planes(const std::vector<fizmo::graphics::ReflectionPlane3D>& planes) { m_planes = planes; }
    bool capsule_shadows() const noexcept { return m_settings.capsule_shadows && shadows(); }
    void clear_occluders() { m_occluders.clear(); }
    void add_occluder(const fizmo::graphics::CapsuleOccluder3D& capsule) { m_occluders.push_back(capsule); }

private:
    static constexpr double SUN_CASTER_REACH = 1.5;

    void environment(const SkyState& sky, double seconds, const Color* camera_medium);

    static constexpr double TIME_WRAP = 3600.0;
    static constexpr double CHANNEL_MAX = 255.0;

    fizmo::graphics::PointLight3D scene_light(const DynamicLight& l, double fade) const;

    void add_dynamic(const vector3d& camera, const std::vector<DynamicLight>& lights);

    void add_blocks(const World& world, const vector3d& camera);

    double rank_fade(double cut, double distance) const noexcept;

    static Color emission_color(const LightEmission& e) noexcept;

    LightingSettings                                m_settings;
    fizmo::graphics::SceneLighting3D                m_scene;
    std::vector<fizmo::graphics::ReflectionPlane3D> m_planes;
    fizmo::graphics::Swell3D                        m_swell;
    std::vector<EmitterBlock>                       m_emitters;
    std::vector<std::pair<double, std::size_t>>     m_order;
    std::vector<fizmo::graphics::CapsuleOccluder3D> m_occluders;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_LIGHTING_DIRECTOR_HPP