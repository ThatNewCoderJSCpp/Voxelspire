#ifndef VOXELSPIRE_SKY_RENDERER_HPP
#define VOXELSPIRE_SKY_RENDERER_HPP

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include "../lighting/sun_path.hpp"
#include "settings.hpp"

namespace voxelspire {

struct CelestialView {
    vector3d eye;
    double   far_plane = 0.0;
    SkyState sky;
    vector3d orbit_axis { 0.0, 0.0, 1.0 };
    double   days      = 0.0;
    double   seconds   = 0.0;
    bool     submerged = false;
};

class CelestialRenderer {
public:
    static constexpr double MIN_DISTANCE = 0.05;
    static constexpr double MAX_DISTANCE = 0.95;
    static constexpr int    MIN_SEGMENTS = 3;
    static constexpr int    MIN_RINGS    = 1;
    static constexpr int    MAX_STARS    = 20000;

    void configure(const CelestialSettings& settings) {
        m_settings = settings;
        m_stars_dirty = true;
    }

    const CelestialSettings& settings() const noexcept { return m_settings; }

    static vector3d orbit_axis(const SunPath& path) {
        const vector3d a = path.evaluate(AXIS_SAMPLE_A).sun_position;
        const vector3d b = path.evaluate(AXIS_SAMPLE_B).sun_position;
        const vector3d n = a.cross(b);
        const double length = n.magnitude();
        return length > EPSILON ? n / length : UP;
    }

    void render(fizmo::windows::Renderer& renderer, const CelestialView& view) {
        const CelestialSettings& s = m_settings;
        if (view.submerged && s.hide_underwater) return;
        const double distance = view.far_plane * vclamp(s.distance, MIN_DISTANCE, MAX_DISTANCE);
        if (distance <= 0.0) return;
        if (s.stars.visible) draw_stars(renderer, view, distance);
        if (s.sun.visible) draw_sun(renderer, view, distance);
        if (s.moon.visible) draw_moon(renderer, view, distance);
    }

private:
    static constexpr double   AXIS_SAMPLE_A = 0.0;
    static constexpr double   AXIS_SAMPLE_B = 0.25;
    static constexpr double   EPSILON       = 1e-9;
    static constexpr double   FULL_TURN     = 2.0 * PI;
    static constexpr double   HALF          = 0.5;
    static constexpr double   CHANNEL       = 255.0;
    static constexpr double   GLOW_FALLOFF  = 2.0;
    static constexpr double   PARALLEL      = 0.999;
    static constexpr vector3d UP { 0.0, 0.0, 1.0 };
    static constexpr vector3d SIDE { 1.0, 0.0, 0.0 };

    struct Rgba { double r = 0.0, g = 0.0, b = 0.0, a = 0.0; };

    struct Basis { vector3d center, u, v; double radius = 0.0; };

    struct Star {
        vector3d direction;
        double   size = 1.0;
        double   brightness = 1.0;
        double   phase = 0.0;
        Color    color;
    };

    static Rgba rgba(const Color& c, double scale, double alpha) noexcept {
        return { c.red() / CHANNEL * scale, c.green() / CHANNEL * scale, c.blue() / CHANNEL * scale, alpha };
    }

    static Color to_color(const Rgba& c) noexcept {
        auto ch = [](double v) { return static_cast<std::uint8_t>(std::lround(vclamp(v, 0.0, 1.0) * CHANNEL)); };
        return Color(ch(c.r), ch(c.g), ch(c.b), ch(c.a));
    }

    static fizmo::graphics::Material3D material(fizmo::graphics::Blend3D blend) noexcept {
        fizmo::graphics::Material3D m = fizmo::graphics::Material3D::transparent();
        m.blend = blend;
        return m.with_lit(false).with_fog(false);
    }

    double horizon(const vector3d& direction) const noexcept {
        const double fade = m_settings.horizon_fade;
        return fade > 0.0 ? smooth_step(-fade, fade, direction.z) : 1.0;
    }

    static vector3d tangent(const vector3d& axis, const vector3d& direction) noexcept {
        vector3d t = axis.cross(direction);
        if (t.magnitude() <= EPSILON) t = (std::fabs(direction.z) < PARALLEL ? UP : SIDE).cross(direction);
        return t / t.magnitude();
    }

    static Basis basis(const vector3d& direction, const vector3d& axis, double distance, double size_degrees) noexcept {
        Basis b;
        b.center = direction * distance;
        b.u      = tangent(axis, direction);
        b.v      = direction.cross(b.u);
        b.radius = distance * std::tan(deg_to_rad(size_degrees * HALF));
        return b;
    }

    template <typename Shade>
    void disc(const Basis& b, int segments, int rings, double soft_edge, Shade shade) {
        using fizmo::graphics::Vertex3D;
        const int n = vmax(segments, MIN_SEGMENTS);
        const int r = vmax(rings, MIN_RINGS);
        const bool feather = soft_edge > 0.0;
        const int total = r + (feather ? 1 : 0);
        auto point = [&b](double x, double y) { return b.center + (b.u * x + b.v * y) * b.radius; };
        m_ring.assign(static_cast<std::size_t>(n), Vertex3D());
        m_previous.assign(static_cast<std::size_t>(n), Vertex3D(point(0.0, 0.0), to_color(shade(0.0, 0.0))));

        for (int k = 1; k <= total; ++k) {
            const bool edge = feather && k == total;
            const double radius = edge ? 1.0 + soft_edge : static_cast<double>(k) / r;
            const double inside = vmin(radius, 1.0);

            for (int j = 0; j < n; ++j) {
                const double angle = FULL_TURN * j / n;
                const double x = std::cos(angle), y = std::sin(angle);
                Rgba c = shade(x * inside, y * inside);
                if (edge) c.a = 0.0;
                m_ring[static_cast<std::size_t>(j)] = Vertex3D(point(x * radius, y * radius), to_color(c));
            }

            for (int j = 0; j < n; ++j) {
                const std::size_t a = static_cast<std::size_t>(j), next = static_cast<std::size_t>((j + 1) % n);
                m_triangles.push_back(m_previous[a]);
                m_triangles.push_back(m_ring[a]);
                m_triangles.push_back(m_ring[next]);
                if (k == 1) continue;
                m_triangles.push_back(m_previous[a]);
                m_triangles.push_back(m_ring[next]);
                m_triangles.push_back(m_previous[next]);
            }

            m_previous.swap(m_ring);
        }
    }

    void flush(fizmo::windows::Renderer& renderer, const vector3d& eye, fizmo::graphics::Blend3D blend) {
        if (!m_triangles.empty()) renderer.draw_triangles_at(m_triangles, eye, material(blend));
        m_triangles.clear();
    }

    void glow(Basis b, const GlowSettings& g, int segments, const Rgba& color, double strength) {
        if (!g.enabled || strength <= 0.0 || g.size <= 1.0) return;
        b.radius *= g.size;
        
        disc(b, segments, g.rings, 0.0, [&color, strength](double x, double y) {
            const double r = std::sqrt(x * x + y * y);
            return Rgba{ color.r, color.g, color.b, strength * std::pow(vmax(1.0 - r, 0.0), GLOW_FALLOFF) };
        });
    }

    void draw_sun(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance) {
        using fizmo::graphics::Blend3D;
        const SunSettings& s = m_settings.sun;
        const vector3d direction = view.sky.sun_position;
        const double fade = horizon(direction);
        if (fade <= 0.0) return;
        const Basis b = basis(direction, view.orbit_axis, distance, s.size);
        const Rgba base = rgba(mix_color(s.color, s.dusk_color, view.sky.dusk), s.brightness, fade);
        glow(b, s.glow, s.segments, base, s.glow.strength * fade);
        flush(renderer, view.eye, Blend3D::Additive);

        disc(b, s.segments, s.rings, s.edge_softness, [&base, &s](double x, double y) {
            const double mu = std::sqrt(vmax(1.0 - x * x - y * y, 0.0));
            const double k = 1.0 - s.limb_darkening * (1.0 - mu);
            return Rgba{ base.r * k, base.g * k, base.b * k, base.a };
        });

        flush(renderer, view.eye, Blend3D::Additive);
    }

    double moon_phase(double days) const noexcept {
        const MoonSettings& m = m_settings.moon;
        if (!m.phases || m.cycle_days <= 0.0) return 0.0;
        const double cycle = days / m.cycle_days + m.phase_offset;
        return FULL_TURN * (cycle - std::floor(cycle));
    }

    void draw_moon(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance) {
        using fizmo::graphics::Blend3D;
        const MoonSettings& m = m_settings.moon;
        const vector3d direction = view.sky.moon_position;
        const double night = view.sky.night;
        const double fade = horizon(direction) * (m.day_visibility + (1.0 - m.day_visibility) * night);
        if (fade <= 0.0) return;
        const Basis b = basis(direction, view.orbit_axis, distance, m.size);
        const double phase = moon_phase(view.days);
        const double lx = std::sin(phase), lz = std::cos(phase);
        const double lit_fraction = (1.0 + lz) * HALF;
        const Rgba base = rgba(m.color, m.brightness, fade);
        glow(b, m.glow, m.segments, base, m.glow.strength * fade * lit_fraction);
        flush(renderer, view.eye, Blend3D::Additive);

        disc(b, m.segments, m.rings, m.edge_softness, [&](double x, double y) {
            const double z = std::sqrt(vmax(1.0 - x * x - y * y, 0.0));
            const double facing = x * lx + z * lz;
            const double terminator = vmax(m.terminator, EPSILON);
            const double lit = smooth_step(0.0, terminator, facing);
            const double light = m.earthshine + (1.0 - m.earthshine) * lit;
            const double alpha = base.a * (lit + (1.0 - lit) * night * m.dark_opacity);
            return Rgba{ base.r * light, base.g * light, base.b * light, alpha };
        });

        flush(renderer, view.eye, Blend3D::Alpha);
    }

    void build_stars() {
        const StarSettings& s = m_settings.stars;
        m_stars.clear();
        m_stars_dirty = false;
        const int count = vclamp(s.count, 0, MAX_STARS);
        m_stars.reserve(static_cast<std::size_t>(count));
        std::mt19937 rng(s.seed);
        std::uniform_real_distribution<double> unit(0.0, 1.0);

        for (int i = 0; i < count; ++i) {
            const double z = 2.0 * unit(rng) - 1.0;
            const double angle = FULL_TURN * unit(rng);
            const double ring = std::sqrt(vmax(1.0 - z * z, 0.0));
            Star star;
            star.direction  = vector3d{ ring * std::cos(angle), ring * std::sin(angle), z };
            star.size       = 1.0 - s.size_variation * unit(rng);
            star.brightness = 1.0 - s.brightness_variation * unit(rng);
            star.phase      = FULL_TURN * unit(rng);
            const double tint = unit(rng) * s.color_variation;
            star.color      = mix_color(s.color, unit(rng) < HALF ? s.warm_color : s.cool_color, tint);
            m_stars.push_back(star);
        }

        m_star_mesh.clear();
        build_octahedron(m_star_mesh);
    }

    static void build_octahedron(fizmo::graphics::Mesh3D& mesh) {
        using fizmo::graphics::Vertex3D;
        const Color white(255, 255, 255);
        const vector3d points[6] = { { 1.0, 0.0, 0.0 }, { -1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, -1.0 } };
        for (const vector3d& p : points) mesh.add_vertex(Vertex3D(p, p, white));
        const std::uint32_t faces[8][3] = { { 0, 2, 4 }, { 2, 1, 4 }, { 1, 3, 4 }, { 3, 0, 4 }, { 2, 0, 5 }, { 1, 2, 5 }, { 3, 1, 5 }, { 0, 3, 5 } };
        for (const auto& f : faces) mesh.add_triangle(f[0], f[1], f[2]);
    }

    static vector3d rotate(const vector3d& v, const vector3d& axis, double cos_a, double sin_a) noexcept {
        return v * cos_a + axis.cross(v) * sin_a + axis * (axis.dot(v) * (1.0 - cos_a));
    }

    void draw_stars(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance) {
        const StarSettings& s = m_settings.stars;
        const double visible = s.brightness * (s.day_visibility + (1.0 - s.day_visibility) * view.sky.night);
        if (visible <= 0.0) return;
        if (m_stars_dirty) build_stars();
        if (m_stars.empty()) return;
        const double angle = s.rotate ? FULL_TURN * view.sky.time : 0.0;
        const double cos_a = std::cos(angle), sin_a = std::sin(angle);
        const double radius = distance * std::tan(deg_to_rad(s.size * HALF));
        const double spin = FULL_TURN * s.twinkle_speed * view.seconds;
        m_instances.clear();

        for (const Star& star : m_stars) {
            const vector3d d = rotate(star.direction, view.orbit_axis, cos_a, sin_a);
            const double fade = horizon(d);
            if (fade <= 0.0) continue;
            const double twinkle = 1.0 - s.twinkle * HALF * (1.0 + std::sin(spin + star.phase));
            const double alpha = visible * star.brightness * fade * twinkle;
            if (alpha <= 0.0) continue;
            const vector3d p = d * distance;

            m_instances.emplace_back(
                static_cast<float>(p.x), 
                static_cast<float>(p.y), 
                static_cast<float>(p.z),
                static_cast<float>(radius * star.size), 
                to_color(rgba(star.color, 1.0, alpha))
            );
        }

        fizmo::graphics::Material3D m = material(fizmo::graphics::Blend3D::Additive);
        m.cull = fizmo::graphics::Cull3D::Back;
        if (!m_instances.empty()) renderer.draw_instances(m_star_mesh, view.eye, m_instances, m);
    }

    CelestialSettings                         m_settings;
    std::vector<fizmo::graphics::Vertex3D>    m_triangles;
    std::vector<fizmo::graphics::Vertex3D>    m_ring;
    std::vector<fizmo::graphics::Vertex3D>    m_previous;
    fizmo::graphics::Mesh3D                   m_star_mesh;
    std::vector<Star>                         m_stars;
    std::vector<fizmo::graphics::Instance3D>  m_instances;
    bool                                      m_stars_dirty = true;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SKY_RENDERER_HPP