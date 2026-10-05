#ifndef VOXELSPIRE_WEATHER_PRECIPITATION_HPP
#define VOXELSPIRE_WEATHER_PRECIPITATION_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../core/random.hpp"
#include "../render/greedy_mesher.hpp"
#include "../world/block_reader.hpp"
#include "system.hpp"

namespace voxelspire {

struct DropStyle {
    vector3d size;
    double   speed  = 10.0;
    double   spread = 0.1;
    double   sway   = 0.0;
    double   drift  = 1.0;
    double   share  = 1.0;
    double   above  = 26.0;
    double   shade  = 1.0;
    Color    color;
};

struct DropStyles {
    static constexpr std::size_t COUNT = 6;

    static std::array<DropStyle, COUNT> builtin() {
        std::array<DropStyle, COUNT> s{};
        s[index(Precipitation::Rain)]         = { { 0.02, 0.02, 0.6 },    14.0, 0.12, 0.0, 1.0, 1.0,  26.0, 1.0,  Color(175, 195, 235, 110) };
        s[index(Precipitation::Snow)]         = { { 0.1, 0.1, 0.1 },       1.6, 0.25, 0.6, 2.2, 1.4,  10.0, 0.8,  Color(255, 255, 255, 235) };
        s[index(Precipitation::Sleet)]        = { { 0.05, 0.05, 0.08 },    8.0, 0.15, 0.0, 1.2, 0.8,  20.0, 0.85, Color(220, 232, 248, 210) };
        s[index(Precipitation::FreezingRain)] = { { 0.02, 0.02, 0.45 },   11.0, 0.12, 0.0, 1.0, 1.0,  24.0, 1.0,  Color(205, 225, 250, 150) };
        s[index(Precipitation::Hail)]         = { { 0.12, 0.12, 0.12 },   17.0, 0.1,  0.0, 0.6, 0.35, 26.0, 0.85, Color(240, 244, 250, 240) };
        return s;
    }

    static std::size_t index(Precipitation p) noexcept { return static_cast<std::size_t>(p); }
};

class PrecipitationRenderer {
public:
    static constexpr double SPAWN_BELOW    = 8.0;
    static constexpr double SCAN_ABOVE     = 64.0;
    static constexpr double FILL_RATE      = 3.0;
    static constexpr double CACHE_SECONDS  = 1.5;
    static constexpr double ESCAPE_MARGIN  = 1.25;
    static constexpr int    ATTEMPTS       = 3;

    PrecipitationRenderer() : m_styles(DropStyles::builtin()) {
        for (std::size_t i = 0; i < DropStyles::COUNT; ++i) m_meshes[i] = box(m_styles[i]);
    }

    std::array<DropStyle, DropStyles::COUNT>& styles() noexcept { return m_styles; }

    void rebuild_meshes() { for (std::size_t i = 0; i < DropStyles::COUNT; ++i) m_meshes[i] = box(m_styles[i]); }

    std::size_t count() const noexcept { return m_drops.size(); }

    void clear() noexcept { m_drops.clear(); m_ceilings.clear(); }

    void update(
        double dt, 
        double seconds, 
        const vector3d& eye, 
        const World& world, 
        const LocalWeather& weather,
        const WeatherViewSettings& view, 
        const vector3d& wind
    ) {
        if (!view.precipitation || weather.precipitation == Precipitation::None) {
            m_drops.clear();
            return;
        }

        m_seconds = seconds;
        m_eye = eye;
        recenter(eye);
        BlockReader reader(world);
        const DropStyle& style = m_styles[DropStyles::index(weather.precipitation)];
        const double radius = view.radius;
        const double target = weather.amount * view.amount * style.share * view.max_drops;
        const float  fdt = static_cast<float>(dt);
        const vector3d local = eye - m_anchor;

        for (std::size_t i = 0; i < m_drops.size();) {
            Drop& d = m_drops[i];
            const DropStyle& ds = m_styles[d.style];
            const float sway = static_cast<float>(ds.sway * std::sin(seconds * SWAY_SPEED + d.phase));
            d.x += (d.vx + sway) * fdt;
            d.y += (d.vy + sway * SWAY_CROSS) * fdt;
            d.z += d.vz * fdt;
            const double dx = d.x - local.x, dy = d.y - local.y;
            const bool escaped = dx * dx + dy * dy > radius * radius * ESCAPE_MARGIN * ESCAPE_MARGIN || d.z < local.z - SPAWN_BELOW;
            const bool landed = d.z <= ceiling(reader, m_anchor.x + d.x, m_anchor.y + d.y, eye.z) + 1.0;
            if (escaped || landed || d.style != DropStyles::index(weather.precipitation)) { m_drops[i] = m_drops.back(); m_drops.pop_back(); continue; }
            ++i;
        }

        const double wanted = vmin(target, static_cast<double>(view.max_drops));
        const std::size_t room = wanted > m_drops.size() ? static_cast<std::size_t>(wanted - m_drops.size()) : 0;
        const std::size_t spawn = vmin(room, static_cast<std::size_t>(std::ceil(wanted * FILL_RATE * dt)) + 1);

        for (std::size_t n = 0; n < spawn; ++n) {
            for (int attempt = 0; attempt < ATTEMPTS; ++attempt) {
                const double angle = m_rng.range(0.0, 2.0 * PI), r = radius * std::sqrt(m_rng.unit());
                const double wx = eye.x + std::cos(angle) * r - wind.x * style.drift, wy = eye.y + std::sin(angle) * r - wind.y * style.drift;
                const double wz = eye.z + m_rng.range(-SPAWN_BELOW, style.above);
                if (wz <= ceiling(reader, wx, wy, eye.z) + 1.0) continue;
                Drop d;
                d.x = static_cast<float>(wx - m_anchor.x);
                d.y = static_cast<float>(wy - m_anchor.y);
                d.z = static_cast<float>(wz - m_anchor.z);
                const double speed = style.speed * (1.0 + m_rng.range(-style.spread, style.spread));
                d.vx = static_cast<float>(wind.x * style.drift);
                d.vy = static_cast<float>(wind.y * style.drift);
                d.vz = static_cast<float>(-speed);
                d.phase = static_cast<float>(m_rng.range(0.0, 2.0 * PI));
                d.style = static_cast<std::uint8_t>(DropStyles::index(weather.precipitation));
                m_drops.push_back(d);
                break;
            }
        }
    }

    void render(fizmo::windows::Renderer& renderer, double brightness) {
        for (auto& list : m_lists) list.clear();
        const vector3d eye = m_eye - m_anchor;
        std::array<std::uint32_t, DropStyles::COUNT> tints{};

        for (std::size_t k = 0; k < DropStyles::COUNT; ++k) {
            const auto level = static_cast<std::uint8_t>(std::lround(vclamp(brightness * m_styles[k].shade, 0.0, 1.0) * CHANNEL));
            tints[k] = fizmo::graphics::Vertex3D::pack(Color(level, level, level));
        }

        for (const Drop& d : m_drops) {
            const double dx = d.x - eye.x, dy = d.y - eye.y, dz = d.z - eye.z;
            if (dx * dx + dy * dy + dz * dz < NEAR_HIDE * NEAR_HIDE) continue;
            fizmo::graphics::Instance3D inst;
            const vector3d& size = m_styles[d.style].size;
            inst.x = d.x - static_cast<float>(size.x * HALF);
            inst.y = d.y - static_cast<float>(size.y * HALF);
            inst.z = d.z;
            inst.scale = 1.0f;
            inst.rgba = tints[d.style];
            m_lists[d.style].push_back(inst);
        }

        fizmo::graphics::Material3D m = fizmo::graphics::Material3D::transparent();
        m.view = fizmo::graphics::View3D::CameraOnly;
        m.lit  = false;

        for (std::size_t k = 0; k < DropStyles::COUNT; ++k)
            if (!m_lists[k].empty()) renderer.draw_instances(m_meshes[k], m_anchor, m_lists[k], m);
    }

private:
    static constexpr double HALF        = 0.5;
    static constexpr double SWAY_SPEED  = 1.7;
    static constexpr double SWAY_CROSS  = 0.6;
    static constexpr double RECENTER    = 256.0;
    static constexpr double NEAR_HIDE   = 1.6;
    static constexpr double CHANNEL     = 255.0;

    struct Drop {
        float        x = 0.0f, y = 0.0f, z = 0.0f;
        float        vx = 0.0f, vy = 0.0f, vz = 0.0f;
        float        phase = 0.0f;
        std::uint8_t style = 0;
    };

    struct Ceiling {
        int    top   = 0;
        double stamp = 0.0;
    };

    static fizmo::graphics::Mesh3D box(const DropStyle& style) {
        using fizmo::graphics::Vertex3D;
        fizmo::graphics::Mesh3D mesh;

        for (Face f : ALL_FACES) {
            std::array<vector3d, 4> q;
            face_corners(vector3d{ 0.0, 0.0, 0.0 }, style.size, f, q);
            const vector3d n = face_normal(f);
            mesh.add_quad(Vertex3D(q[0], n, style.color), Vertex3D(q[1], n, style.color), Vertex3D(q[2], n, style.color), Vertex3D(q[3], n, style.color));
        }

        return mesh;
    }

    void recenter(const vector3d& eye) {
        const vector3d d = eye - m_anchor;
        if (std::fabs(d.x) < RECENTER && std::fabs(d.y) < RECENTER && std::fabs(d.z) < RECENTER) return;
        const vector3d shift{ std::floor(d.x), std::floor(d.y), std::floor(d.z) };

        for (Drop& drop : m_drops) {
            drop.x -= static_cast<float>(shift.x);
            drop.y -= static_cast<float>(shift.y);
            drop.z -= static_cast<float>(shift.z);
        }

        m_anchor += shift;
    }

    double ceiling(BlockReader& reader, double x, double y, double eye_z) {
        const int bx = static_cast<int>(std::floor(x)), by = static_cast<int>(std::floor(y));
        const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(bx)) << 32) | static_cast<std::uint32_t>(by);
        auto it = m_ceilings.find(key);
        if (it != m_ceilings.end() && m_seconds - it->second.stamp < CACHE_SECONDS) return it->second.top;
        const int high = static_cast<int>(std::floor(eye_z + SCAN_ABOVE)), low = static_cast<int>(std::floor(eye_z - SPAWN_BELOW)) - 1;
        int top = low;

        for (int z = high; z > low; --z) {
            const BlockTraits& t = reader.traits_at({ bx, by, z });
            if (t.solid || t.fluid || t.opaque) { top = z; break; }
        }

        m_ceilings[key] = { top, m_seconds };
        if (m_ceilings.size() > MAX_CACHED) prune();
        return top;
    }

    void prune() {
        for (auto it = m_ceilings.begin(); it != m_ceilings.end();) {
            if (m_seconds - it->second.stamp >= CACHE_SECONDS) it = m_ceilings.erase(it);
            else ++it;
        }
    }

    static constexpr std::size_t MAX_CACHED = 16384;

    std::array<DropStyle, DropStyles::COUNT>                             m_styles;
    std::array<fizmo::graphics::Mesh3D, DropStyles::COUNT>               m_meshes;
    std::array<std::vector<fizmo::graphics::Instance3D>, DropStyles::COUNT> m_lists;
    std::vector<Drop>                                                    m_drops;
    std::unordered_map<std::uint64_t, Ceiling>                           m_ceilings;
    SeededRandom                                                         m_rng;
    vector3d                                                             m_anchor{};
    vector3d                                                             m_eye{};
    double                                                               m_seconds = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WEATHER_PRECIPITATION_HPP