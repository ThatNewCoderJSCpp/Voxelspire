#ifndef VOXELSPIRE_WORLD_CAVES_HPP
#define VOXELSPIRE_WORLD_CAVES_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "../core/noise.hpp"
#include "../core/random.hpp"
#include "chunk.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

struct CaveEntrance {
    double x = 0.0;
    double y = 0.0;
    double top    = 0.0;
    double dir_x  = 1.0;
    double dir_y = 0.0;
    double slope  = 1.0;
    double radius = 2.0;
    double depth  = 32.0;

    bool contains(double px, double py, double pz) const noexcept {
        const double d = top - pz;
        if (d < -radius || d > depth) return false;
        const double along = vclamp(d, 0.0, depth);
        const double cx = x + dir_x * slope * along, cy = y + dir_y * slope * along;
        const double r = radius * (1.0 - NARROWING * along / depth);
        const double dx = px - cx, dy = py - cy;
        return dx * dx + dy * dy <= r * r;
    }

    double reach() const noexcept { return depth * slope + radius; }

private:
    static constexpr double NARROWING = 0.3;
};

class CaveCarver {
public:
    static constexpr int    STEP              = 4;
    static constexpr double TUNNEL_SCALE      = 72.0;
    static constexpr double CHAMBER_SCALE     = 110.0;
    static constexpr double FLATTEN           = 1.8;
    static constexpr double TUNNEL_WIDTH      = 0.075;
    static constexpr double CHAMBER_LEVEL     = 0.55;
    static constexpr double CHAMBER_STEP      = 0.08;
    static constexpr double DEEP_RANGE        = 96.0;
    static constexpr int    TUNNEL_OCTAVES    = 2;
    static constexpr int    CHAMBER_OCTAVES   = 3;
    static constexpr double ENTRANCE_CELL     = 56.0;
    static constexpr double ENTRANCE_CHANCE   = 0.7;
    static constexpr double ENTRANCE_RADIUS   = 3.0;
    static constexpr double ENTRANCE_SPREAD   = 1.2;
    static constexpr double SLOPE_MIN         = 0.35;
    static constexpr double SLOPE_MAX         = 1.1;
    static constexpr double DEPTH_SPREAD      = 0.4;
    static constexpr double CANYON_SCALE      = 260.0;
    static constexpr double CANYON_MASK_SCALE = 420.0;
    static constexpr double CANYON_WIDTH      = 0.03;
    static constexpr double CANYON_MASK       = 0.55;
    static constexpr double CANYON_MASK_STEP  = 0.2;
    static constexpr double CANYON_MASK_FADE  = 0.15;
    static constexpr int    CANYON_OCTAVES    = 3;

    CaveCarver(std::uint64_t seed, const CaveSettings& s)
        : m_settings(s),
          m_enabled(s.enabled),
          m_tunnels(s.enabled && s.tunnels > 0.0),
          m_caverns(s.enabled && s.caverns > 0.0),
          m_squash(FLATTEN * vmax(s.flatness, MIN_SCALE)),
          m_tunnel_a(seed ^ TUNNEL_A_SALT, { tunnel_scale(s), TUNNEL_OCTAVES }),
          m_tunnel_b(seed ^ TUNNEL_B_SALT, { tunnel_scale(s), TUNNEL_OCTAVES }),
          m_chamber(seed ^ CHAMBER_SALT, { CHAMBER_SCALE * vmax(s.cavern_size, MIN_SCALE), CHAMBER_OCTAVES }),
          m_canyon(seed ^ CANYON_SALT, { CANYON_SCALE * vmax(s.canyon_width, MIN_SCALE), CANYON_OCTAVES }),
          m_canyon_mask(seed ^ CANYON_MASK_SALT, { CANYON_MASK_SCALE, CANYON_OCTAVES }),
          m_width(TUNNEL_WIDTH * vmax(s.tunnel_width, 0.0) * std::sqrt(vmax(s.tunnels, MIN_SCALE)) / vmax(s.tunnel_length, MIN_SCALE)),
          m_chamber_level(CHAMBER_LEVEL - CHAMBER_STEP * (s.caverns - 1.0)),
          m_canyon_width(CANYON_WIDTH * vmax(s.canyon_width, 0.0)),
          m_canyon_mask_level(CANYON_MASK - CANYON_MASK_STEP * (s.canyons - 1.0)),
          m_seed(seed) {}

    bool enabled() const noexcept { return m_enabled; }
    const CaveSettings& settings() const noexcept { return m_settings; }

    struct Grid {
        static constexpr int N = Chunk::SIZE / STEP + 1;
        std::array<float, N * N * N> tunnel{};
        std::array<float, N * N * N> chamber{};

        static constexpr int index(int x, int y, int z) noexcept { return (z * N + y) * N + x; }
    };

    void fill(Grid& g, const BlockPos& origin) const noexcept {
        if (!m_tunnels && !m_caverns) return;

        for (int z = 0; z < Grid::N; ++z)
            for (int y = 0; y < Grid::N; ++y)
                for (int x = 0; x < Grid::N; ++x) {
                    const double wx = origin.x + x * STEP, wy = origin.y + y * STEP, wz = (origin.z + z * STEP) * m_squash;
                    const int i = Grid::index(x, y, z);

                    if (m_tunnels) {
                        const double a = m_tunnel_a.at(wx, wy, wz), b = m_tunnel_b.at(wx, wy, wz);
                        g.tunnel[static_cast<std::size_t>(i)] = static_cast<float>(std::sqrt(a * a + b * b));
                    }

                    if (m_caverns) g.chamber[static_cast<std::size_t>(i)] = static_cast<float>(m_chamber.at(wx, wy, wz));
                }
    }

    bool hollow(const Grid& g, int lx, int ly, int lz, int depth) const noexcept {
        if (depth < m_settings.min_depth) return false;
        if (m_settings.max_depth > 0 && depth > m_settings.max_depth) return false;
        const double grow = 1.0 + m_settings.deep_growth * vclamp(depth / DEEP_RANGE, 0.0, 1.0);
        if (m_tunnels && sample(g.tunnel, lx, ly, lz) < m_width * grow) return true;
        return m_caverns && depth > m_settings.cavern_roof && sample(g.chamber, lx, ly, lz) > m_chamber_level / grow;
    }

    double canyon(double x, double y) const noexcept {
        if (!m_enabled || m_settings.canyons <= 0.0 || m_canyon_width <= 0.0) return 0.0;
        const double mask = m_canyon_mask.at(x, y);
        if (mask <= m_canyon_mask_level) return 0.0;
        const double n = std::fabs(m_canyon.at(x, y));
        if (n >= m_canyon_width) return 0.0;
        const double fade = vclamp((mask - m_canyon_mask_level) / CANYON_MASK_FADE, 0.0, 1.0);
        return m_settings.canyon_depth * std::sqrt(1.0 - n / m_canyon_width) * fade;
    }

    template <typename Height>
    void entrances(int x0, int y0, int x1, int y1, Height&& height, int sea_level, std::vector<CaveEntrance>& out) const {
        out.clear();
        if (!m_enabled || m_settings.entrances <= 0.0) return;
        const double cell = ENTRANCE_CELL / std::sqrt(m_settings.entrances);
        const double reach = m_settings.entrance_depth * (1.0 + DEPTH_SPREAD) * SLOPE_MAX + ENTRANCE_RADIUS * ENTRANCE_SPREAD * vmax(m_settings.entrance_width, MIN_SCALE);
        const int cx0 = static_cast<int>(std::floor((x0 - reach) / cell)), cx1 = static_cast<int>(std::floor((x1 + reach) / cell));
        const int cy0 = static_cast<int>(std::floor((y0 - reach) / cell)), cy1 = static_cast<int>(std::floor((y1 + reach) / cell));

        for (int cy = cy0; cy <= cy1; ++cy)
            for (int cx = cx0; cx <= cx1; ++cx) {
                SeededRandom rng = SeededRandom::at(m_seed, cx, cy, 0, ENTRANCE_SALT);
                if (rng.unit() >= ENTRANCE_CHANCE) continue;
                CaveEntrance e;
                e.x = (cx + rng.unit()) * cell;
                e.y = (cy + rng.unit()) * cell;
                const double angle = rng.range(0.0, 2.0 * PI);
                e.dir_x  = std::cos(angle);
                e.dir_y  = std::sin(angle);
                e.slope  = rng.range(SLOPE_MIN, SLOPE_MAX);
                e.radius = ENTRANCE_RADIUS * vmax(m_settings.entrance_width, MIN_SCALE) * rng.range(1.0, ENTRANCE_SPREAD);
                e.depth  = m_settings.entrance_depth * rng.range(1.0 - DEPTH_SPREAD, 1.0 + DEPTH_SPREAD);
                e.top    = height(static_cast<int>(std::floor(e.x)), static_cast<int>(std::floor(e.y)));
                if (e.top < sea_level) continue;
                out.push_back(e);
            }
    }

private:
    static constexpr double        MIN_SCALE        = 0.05;
    static constexpr std::uint64_t TUNNEL_A_SALT    = 0xC0FFEE01ull;
    static constexpr std::uint64_t TUNNEL_B_SALT    = 0xC0FFEE02ull;
    static constexpr std::uint64_t CHAMBER_SALT     = 0xC0FFEE03ull;
    static constexpr std::uint64_t CANYON_SALT      = 0xC0FFEE04ull;
    static constexpr std::uint64_t CANYON_MASK_SALT = 0xC0FFEE05ull;
    static constexpr std::uint64_t ENTRANCE_SALT    = 0xC0FFEE06ull;

    static double tunnel_scale(const CaveSettings& s) noexcept {
        return TUNNEL_SCALE * vmax(s.tunnel_length, MIN_SCALE) / std::sqrt(vmax(s.tunnels, MIN_SCALE));
    }

    static float sample(const std::array<float, Grid::N * Grid::N * Grid::N>& v, int lx, int ly, int lz) noexcept {
        const int x0 = lx / STEP, y0 = ly / STEP, z0 = lz / STEP;
        const float fx = static_cast<float>(lx % STEP) / STEP, fy = static_cast<float>(ly % STEP) / STEP, fz = static_cast<float>(lz % STEP) / STEP;
        auto at = [&](int dx, int dy, int dz) { return v[static_cast<std::size_t>(Grid::index(x0 + dx, y0 + dy, z0 + dz))]; };
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        const float c00 = lerp(at(0, 0, 0), at(1, 0, 0), fx), c10 = lerp(at(0, 1, 0), at(1, 1, 0), fx);
        const float c01 = lerp(at(0, 0, 1), at(1, 0, 1), fx), c11 = lerp(at(0, 1, 1), at(1, 1, 1), fx);
        return lerp(lerp(c00, c10, fy), lerp(c01, c11, fy), fz);
    }

    CaveSettings  m_settings;
    bool          m_enabled;
    bool          m_tunnels;
    bool          m_caverns;
    double        m_squash;
    FractalNoise  m_tunnel_a, m_tunnel_b, m_chamber, m_canyon, m_canyon_mask;
    double        m_width;
    double        m_chamber_level;
    double        m_canyon_width;
    double        m_canyon_mask_level;
    std::uint64_t m_seed;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CAVES_HPP