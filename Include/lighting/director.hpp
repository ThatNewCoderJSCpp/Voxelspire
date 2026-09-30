#ifndef VOXELSPIRE_LIGHTING_LIGHTING_DIRECTOR_HPP
#define VOXELSPIRE_LIGHTING_LIGHTING_DIRECTOR_HPP

#include <algorithm>
#include <cmath>
#include <vector>
#include "../world/world.hpp"
#include "dynamic_light.hpp"
#include "settings.hpp"
#include "sun_path.hpp"

namespace voxelspire {

class LightingDirector {
public:
    void configure(const LightingSettings& settings) { m_settings = settings; }
    const LightingSettings& settings() const noexcept { return m_settings; }

    bool enabled() const noexcept {
        return m_settings.baked_light || m_settings.sun_lighting || m_settings.dynamic_lights || m_settings.block_point_lights;
    }

    bool shadows() const noexcept { return enabled() && ((m_settings.sun_shadows && m_settings.sun_lighting) || m_settings.point_shadows); }

    double shadow_distance() const noexcept {
        double d = 0.0;
        if (m_settings.sun_shadows && m_settings.sun_lighting) d = vmax(d, m_settings.sun_shadow_distance * SUN_CASTER_REACH);
        if (m_settings.point_shadows && m_settings.block_point_lights) d = vmax(d, m_settings.block_point_light_distance + m_settings.block_point_light_radius);
        if (m_settings.point_shadows && m_settings.dynamic_lights) d = vmax(d, m_settings.dynamic_light_distance);
        return d;
    }

    const fizmo::graphics::SceneLighting3D& build(
        const SkyState& sky, 
        const World& world, 
        const vector3d& camera, 
        const std::vector<DynamicLight>& lights,
        double seconds = 0.0, 
        const Color* camera_medium = nullptr
    ) {
        using namespace fizmo::graphics;
        const LightingSettings& s = m_settings;
        SceneLighting3D& out = m_scene;
        out.enabled = enabled();
        out.point_lights.clear();
        if (!out.enabled) return out;

        out.sun_direction   = sky.light_direction;
        out.sun_color       = sky.light_color;
        out.sun_intensity   = s.sun_lighting ? static_cast<float>(s.sun_strength * sky.light_strength) : 0.0f;
        out.sun_exposure    = static_cast<float>(s.sun_exposure);
        out.sky_color       = sky.sky_light_color;
        out.sky_intensity   = static_cast<float>(s.sky_light * sky.daylight);
        out.block_color     = s.block_tint;
        out.block_intensity = static_cast<float>(s.block_light);
        out.ambient         = static_cast<float>(s.ambient);
        out.min_light       = static_cast<float>(s.min_light);
        out.max_light       = static_cast<float>(s.max_light);
        out.falloff         = static_cast<float>(s.falloff);

        out.sun_shadow.enabled    = s.sun_shadows && s.sun_lighting;
        out.sun_shadow.resolution = s.sun_shadow_resolution;
        out.sun_shadow.distance   = s.sun_shadow_distance;
        out.sun_shadow.strength   = static_cast<float>(s.shadow_strength);
        out.sun_shadow.softness   = static_cast<float>(s.shadow_softness);
        out.sun_shadow.angle_step = s.shadow_angle_step;
        out.sun_shadow.crossfade  = s.shadow_crossfade;
        out.sun_shadow.soft         = s.soft_shadows;
        out.sun_shadow.light_size   = static_cast<float>(s.soft_shadow_sun_size);
        out.sun_shadow.max_softness = static_cast<float>(s.max_shadow_softness);
        out.sun_shadow.filter_taps  = s.shadow_filter_taps;

        out.point_shadows.enabled    = s.point_shadows;
        out.point_shadows.max_lights = s.max_point_shadows;
        out.point_shadows.resolution = s.point_shadow_resolution;
        out.point_shadows.strength   = static_cast<float>(s.shadow_strength);
        out.point_shadows.softness   = static_cast<float>(s.shadow_softness);
        out.point_shadows.fade_distance   = s.point_shadow_fade;
        out.point_shadows.hide_unshadowed = s.hide_unshadowed_point_lights;

        environment(sky, seconds, camera_medium);
        if (s.dynamic_lights) add_dynamic(camera, lights);
        if (s.block_point_lights) add_blocks(world, camera);
        return out;
    }

    LightLevel light_level_at(const World& world, const vector3d& p, const std::vector<DynamicLight>& lights) const {
        LightLevel level = world.light_at(BlockPos::containing(p));

        for (const DynamicLight& l : lights) {
            if (!l.affects_light_level) continue;
            const vector3d d = l.position - p;
            const int reach = l.level - static_cast<int>(std::floor(std::fabs(d.x) + std::fabs(d.y) + std::fabs(d.z)));
            if (reach <= 0) continue;
            auto scaled = [reach](std::uint8_t c) { return static_cast<int>(std::lround(reach * c / static_cast<double>(CHANNEL_MAX))); };
            level.raise_block(scaled(l.color.red()), scaled(l.color.green()), scaled(l.color.blue()));
        }

        return level;
    }

    fizmo::graphics::BakedLight render_light_at(const World& world, const vector3d& p) const {
        if (!m_settings.baked_light) return fizmo::graphics::BakedLight::full_sky();
        return world.light_at(BlockPos::containing(p)).baked();
    }

    const fizmo::graphics::SceneLighting3D& scene() const noexcept { return m_scene; }

    void set_reflection_planes(const std::vector<fizmo::graphics::ReflectionPlane3D>& planes) { m_planes = planes; }

private:
    static constexpr double SUN_CASTER_REACH = 1.5;

    void environment(const SkyState& sky, double seconds, const Color* camera_medium) {
        using namespace fizmo::graphics;
        const LightingSettings& s = m_settings;
        SceneLighting3D& out = m_scene;
        Atmosphere3D& a = out.atmosphere;
        a.enabled       = s.atmosphere;
        a.sun_position  = sky.sun_position;
        a.moon_position = sky.moon_position;
        a.zenith        = sky.zenith;
        a.horizon       = sky.horizon;
        a.glow          = sky.glow;
        a.glow_strength = static_cast<float>(s.sky_glow * (1.0 - sky.night));
        a.sun_disk      = 0.0f;
        a.moon_disk     = 0.0f;
        a.stars         = 0.0f;
        a.glow_spread   = static_cast<float>(s.sun_glow_spread);
        a.glow_focus    = static_cast<float>(s.sun_glow_focus);
        a.fog_density   = static_cast<float>(s.fog_density);
        a.fog_start     = static_cast<float>(s.fog_start);

        Volumetrics3D& v = out.volumetrics;
        v.enabled    = s.volumetric_light && s.sun_lighting;
        v.steps      = s.volumetric_steps;
        v.density    = static_cast<float>(s.volumetric_density);
        v.anisotropy = static_cast<float>(s.volumetric_anisotropy);
        v.distance   = static_cast<float>(s.volumetric_distance);
        v.intensity  = static_cast<float>(s.volumetric_intensity);
        v.near_bias  = static_cast<float>(s.volumetric_near_bias);

        LightShafts3D& r = out.shafts;
        r.enabled  = s.light_shafts && s.sun_lighting;
        r.samples  = s.light_shaft_samples;
        r.strength = static_cast<float>(s.light_shaft_strength * sky.light_strength);
        r.decay    = static_cast<float>(s.light_shaft_decay);
        r.length   = static_cast<float>(s.light_shaft_length);
        r.focus    = static_cast<float>(s.light_shaft_focus);

        PlanarReflections3D& p = out.planar;
        p.enabled    = s.planar_reflections;
        p.max_planes = s.max_reflection_planes;
        p.resolution = static_cast<float>(s.reflection_resolution);
        p.distortion = static_cast<float>(s.reflection_distortion);
        p.planes     = m_planes;

        Surfaces3D& f = out.surfaces;
        f.reflectivity   = s.reflections ? static_cast<float>(s.reflectivity) : 0.0f;
        f.specular       = s.reflections ? static_cast<float>(s.specular) : 0.0f;
        f.wave_strength  = static_cast<float>(s.wave_strength);
        f.wave_scale     = static_cast<float>(s.wave_scale);
        f.wave_speed     = static_cast<float>(s.wave_speed);
        f.specular_power = static_cast<float>(s.specular_power);
        f.refraction          = s.refraction;
        f.refraction_strength = static_cast<float>(s.refraction_strength);
        f.absorption          = static_cast<float>(s.water_absorption);
        f.scattering          = static_cast<float>(s.water_scattering);
        f.screen_reflections  = s.screen_reflections;
        f.reflection_steps    = s.reflection_steps;
        f.reflection_distance = static_cast<float>(s.reflection_distance);

        out.medium.active  = s.underwater_fog && camera_medium != nullptr;
        out.medium.density = static_cast<float>(s.underwater_density);
        if (camera_medium) out.medium.color = mix_color(*camera_medium, Color(0, 0, 0), 1.0 - sky.daylight);

        out.tone_map.enabled    = s.tone_mapping;
        out.tone_map.exposure   = static_cast<float>(s.exposure);
        out.tone_map.saturation = static_cast<float>(s.saturation);
        out.time = std::fmod(seconds, TIME_WRAP);
    }

    static constexpr double TIME_WRAP = 3600.0;
    static constexpr double CHANNEL_MAX = 255.0;

    void add_dynamic(const vector3d& camera, const std::vector<DynamicLight>& lights) {
        m_order.clear();
        const double reach = m_settings.dynamic_light_distance;

        for (std::size_t i = 0; i < lights.size(); ++i) {
            const vector3d d = lights[i].position - camera;
            const double dist = d.magnitude() - lights[i].radius;
            if (dist <= reach) m_order.emplace_back(dist, i);
        }

        const std::size_t n = vmin(m_order.size(), m_settings.max_dynamic_lights);
        std::sort(m_order.begin(), m_order.end());
        const double cut = n < m_order.size() ? m_order[n].first : reach;

        for (std::size_t k = 0; k < n; ++k) {
            const DynamicLight& l = lights[m_order[k].second];
            const double fade = rank_fade(cut, m_order[k].first);
            m_scene.point_lights.emplace_back(l.position, l.color, static_cast<float>(l.intensity * fade), static_cast<float>(l.radius), l.casts_shadows || m_settings.point_shadows);
        }
    }

    void add_blocks(const World& world, const vector3d& camera) {
        const LightEngine* engine = world.lighting();
        if (!engine) return;
        m_emitters.clear();
        engine->collect_emitters(camera, m_settings.block_point_light_distance, m_emitters);
        m_order.clear();

        for (std::size_t i = 0; i < m_emitters.size(); ++i) {
            const vector3d d = m_emitters[i].pos.center() - camera;
            m_order.emplace_back(d.magnitude(), i);
        }

        const std::size_t n = vmin(m_order.size(), m_settings.max_block_point_lights);
        std::sort(m_order.begin(), m_order.end());
        const bool colored = world.light_format() == LightFormat::Colored;
        const double cut = n < m_order.size() ? m_order[n].first : m_settings.block_point_light_distance;

        for (std::size_t k = 0; k < n; ++k) {
            const EmitterBlock& e = m_emitters[m_order[k].second];
            const double strength = static_cast<double>(e.light.level()) / LightLimits::MAX;
            const Color color = colored ? emission_color(e.light) : m_settings.block_tint;
            const double fade = rank_fade(cut, m_order[k].first);
            m_scene.point_lights.emplace_back(e.pos.center(), color, static_cast<float>(m_settings.block_point_light_intensity * strength * fade),
                                              static_cast<float>(m_settings.block_point_light_radius * strength), m_settings.point_shadows);
        }
    }

    double rank_fade(double cut, double distance) const noexcept {
        const double band = m_settings.point_light_fade;
        if (band <= 0.0) return distance < cut ? 1.0 : 0.0;
        return vclamp((cut - distance) / band, 0.0, 1.0);
    }

    static Color emission_color(const LightEmission& e) noexcept {
        const double peak = vmax(e.level(), 1);
        auto ch = [peak](int v) { return static_cast<std::uint8_t>(std::lround(CHANNEL_MAX * v / peak)); };
        return Color(ch(e.red), ch(e.green), ch(e.blue));
    }

    LightingSettings                        m_settings;
    fizmo::graphics::SceneLighting3D        m_scene;
    std::vector<fizmo::graphics::ReflectionPlane3D> m_planes;
    std::vector<EmitterBlock>               m_emitters;
    std::vector<std::pair<double, std::size_t>> m_order;
};

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_LIGHTING_DIRECTOR_HPP