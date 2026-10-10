#include "sky/renderer.hpp"

namespace voxelspire {

vector3d CelestialRenderer::orbit_axis(const SunPath& path) {
    const vector3d a = path.evaluate(AXIS_SAMPLE_A).sun_position;
    const vector3d b = path.evaluate(AXIS_SAMPLE_B).sun_position;
    const vector3d n = a.cross(b);
    const double length = n.magnitude();
    return length > EPSILON ? n / length : UP;
}

void CelestialRenderer::render(fizmo::windows::Renderer& renderer, const CelestialView& view) {
    const CelestialSettings& s = m_settings;
    if (view.submerged && s.hide_underwater) return;
    const double distance = view.far_plane * CelestialLimits::distance.clamp(s.distance);
    if (distance <= 0.0) return;
    if (s.stars.visible) draw_stars(renderer, view, distance);
    if (s.sun.visible) draw_sun(renderer, view, distance);
    if (s.moon.visible) draw_moon(renderer, view, distance);
}

Color CelestialRenderer::to_color(const Rgba& c) noexcept {
    auto ch = [](double v) { return static_cast<std::uint8_t>(std::lround(vclamp(v, 0.0, 1.0) * CHANNEL)); };
    return Color(ch(c.r), ch(c.g), ch(c.b), ch(c.a));
}

fizmo::graphics::Material3D CelestialRenderer::material(fizmo::graphics::Blend3D blend) noexcept {
    fizmo::graphics::Material3D m = fizmo::graphics::Material3D::transparent();
    m.blend = blend;
    return m.with_lit(false).with_fog(false);
}

double CelestialRenderer::horizon(const vector3d& direction) const noexcept {
    const double fade = m_settings.horizon_fade;
    return fade > 0.0 ? smooth_step(-fade, fade, direction.z) : 1.0;
}

vector3d CelestialRenderer::tangent(const vector3d& axis, const vector3d& direction) noexcept {
    vector3d t = axis.cross(direction);
    if (t.magnitude() <= EPSILON) t = (std::fabs(direction.z) < PARALLEL ? UP : SIDE).cross(direction);
    return t / t.magnitude();
}

auto CelestialRenderer::basis(const vector3d& direction, const vector3d& axis, double distance, double size_degrees) noexcept -> Basis {
    Basis b;
    b.center = direction * distance;
    b.u      = tangent(axis, direction);
    b.v      = direction.cross(b.u);
    b.radius = distance * std::tan(deg_to_rad(size_degrees * HALF));
    return b;
}

void CelestialRenderer::flush(fizmo::windows::Renderer& renderer, const vector3d& eye, fizmo::graphics::Blend3D blend) {
    if (!m_triangles.empty()) renderer.draw_triangles_at(m_triangles, eye, material(blend));
    m_triangles.clear();
}

void CelestialRenderer::glow(Basis b, const GlowSettings& g, int segments, const Rgba& color, double strength) {
    if (!g.enabled || strength <= 0.0 || g.size <= 1.0) return;
    b.radius *= g.size;
        
    disc(b, segments, g.rings, 0.0, [&color, strength](double x, double y) {
        const double r = std::sqrt(x * x + y * y);
        return Rgba{ color.r, color.g, color.b, strength * std::pow(vmax(1.0 - r, 0.0), GLOW_FALLOFF) };
    });
}

void CelestialRenderer::draw_sun(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance) {
    using fizmo::graphics::Blend3D;
    const SunSettings& s = m_settings.sun;
    const vector3d direction = view.sky.sun_position;
    const double fade = horizon(direction) * view.sky.clear;
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

double CelestialRenderer::moon_phase(double days) const noexcept {
    const MoonSettings& m = m_settings.moon;
    if (!m.phases || m.cycle_days <= 0.0) return 0.0;
    const double cycle = days / m.cycle_days + m.phase_offset;
    return FULL_TURN * (cycle - std::floor(cycle));
}

void CelestialRenderer::draw_moon(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance) {
    using fizmo::graphics::Blend3D;
    const MoonSettings& m = m_settings.moon;
    const vector3d direction = view.sky.moon_position;
    const double night = view.sky.night;
    const double fade = horizon(direction) * (m.day_visibility + (1.0 - m.day_visibility) * night) * view.sky.clear;
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

void CelestialRenderer::build_stars() {
    const StarSettings& s = m_settings.stars;
    m_stars.clear();
    m_stars_dirty = false;
    const int count = static_cast<int>(StarLimits::count.clamp(s.count));
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

void CelestialRenderer::build_octahedron(fizmo::graphics::Mesh3D& mesh) {
    using fizmo::graphics::Vertex3D;
    const Color white(255, 255, 255);
    const vector3d points[6] = { { 1.0, 0.0, 0.0 }, { -1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 0.0, -1.0 } };
    for (const vector3d& p : points) mesh.add_vertex(Vertex3D(p, p, white));
    const std::uint32_t faces[8][3] = { { 0, 2, 4 }, { 2, 1, 4 }, { 1, 3, 4 }, { 3, 0, 4 }, { 2, 0, 5 }, { 1, 2, 5 }, { 3, 1, 5 }, { 0, 3, 5 } };
    for (const auto& f : faces) mesh.add_triangle(f[0], f[1], f[2]);
}

void CelestialRenderer::draw_stars(fizmo::windows::Renderer& renderer, const CelestialView& view, double distance) {
    const StarSettings& s = m_settings.stars;
    const double visible = s.brightness * (s.day_visibility + (1.0 - s.day_visibility) * view.sky.night) * view.sky.clear;
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

} // namespace voxelspire
