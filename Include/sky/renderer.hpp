#ifndef VOXELSPIRE_SKY_RENDERER_HPP
#define VOXELSPIRE_SKY_RENDERER_HPP

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include "../lighting/sun_path.hpp"
#include "../core/settings.hpp"

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
    static constexpr int MIN_SEGMENTS = static_cast<int>(SunLimits::segments.min);
    static constexpr int MIN_RINGS    = static_cast<int>(SunLimits::rings.min);

    void configure(const CelestialSettings& settings) {
        m_settings = settings;
        m_stars_dirty = true;
    }

    const CelestialSettings& settings() const noexcept { return m_settings; }

    static vector3d orbit_axis(const SunPath& path);

    void render(fizmo::windows::Renderer& renderer, const CelestialView& view);

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

    static Color to_color(const Rgba& c) noexcept;

    static fizmo::graphics::Material3D material(fizmo::graphics::Blend3D blend) noexcept;

    double horizon(const vector3d& direction) const noexcept;

    static vector3d tangent(const vector3d& axis, const vector3d& direction) noexcept;

    static Basis basis(const vector3d& direction, const vector3d& axis, double distance, double size_degrees) noexcept;

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

    void flush(fizmo::windows::Renderer& renderer, const vector3d& eye, fizmo::graphics::Blend3D blend);

    void glow(Basis b, const GlowSettings& g, int segments, const Rgba& color, double strength);

    void draw_sun(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance);

    double moon_phase(double days) const noexcept;

    void draw_moon(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance);

    void build_stars();

    static void build_octahedron(fizmo::graphics::Mesh3D& mesh);

    static vector3d rotate(const vector3d& v, const vector3d& axis, double cos_a, double sin_a) noexcept {
        return v * cos_a + axis.cross(v) * sin_a + axis * (axis.dot(v) * (1.0 - cos_a));
    }

    void draw_stars(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance);

    CelestialSettings                        m_settings;
    std::vector<fizmo::graphics::Vertex3D>   m_triangles;
    std::vector<fizmo::graphics::Vertex3D>   m_ring;
    std::vector<fizmo::graphics::Vertex3D>   m_previous;
    fizmo::graphics::Mesh3D                  m_star_mesh;
    std::vector<Star>                        m_stars;
    std::vector<fizmo::graphics::Instance3D> m_instances;
    bool                                     m_stars_dirty = true;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SKY_RENDERER_HPP