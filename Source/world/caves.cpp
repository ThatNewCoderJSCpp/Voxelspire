#include "world/caves.hpp"

namespace voxelspire {

bool CaveEntrance::contains(double px, double py, double pz) const noexcept {
    const double d = top - pz;
    if (d < -radius || d > depth) return false;
    const double along = vclamp(d, 0.0, depth);
    const double cx = x + dir_x * slope * along, cy = y + dir_y * slope * along;
    const double r = radius * (1.0 - NARROWING * along / depth);
    const double dx = px - cx, dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}

CaveCarver::CaveCarver(std::uint64_t seed, const CaveSettings& s) : m_settings(s),
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

void CaveCarver::fill(Grid& g, const BlockPos& origin) const noexcept {
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

bool CaveCarver::hollow(const Grid& g, int lx, int ly, int lz, int depth) const noexcept {
    if (depth < m_settings.min_depth) return false;
    if (m_settings.max_depth > 0 && depth > m_settings.max_depth) return false;
    const double grow = 1.0 + m_settings.deep_growth * vclamp(depth / DEEP_RANGE, 0.0, 1.0);
    if (m_tunnels && sample(g.tunnel, lx, ly, lz) < m_width * grow) return true;
    return m_caverns && depth > m_settings.cavern_roof && sample(g.chamber, lx, ly, lz) > m_chamber_level / grow;
}

double CaveCarver::canyon(double x, double y) const noexcept {
    if (!m_enabled || m_settings.canyons <= 0.0 || m_canyon_width <= 0.0) return 0.0;
    const double mask = m_canyon_mask.at(x, y);
    if (mask <= m_canyon_mask_level) return 0.0;
    const double n = std::fabs(m_canyon.at(x, y));
    if (n >= m_canyon_width) return 0.0;
    const double fade = vclamp((mask - m_canyon_mask_level) / CANYON_MASK_FADE, 0.0, 1.0);
    return m_settings.canyon_depth * std::sqrt(1.0 - n / m_canyon_width) * fade;
}

double CaveCarver::tunnel_scale(const CaveSettings& s) noexcept {
    return TUNNEL_SCALE * vmax(s.tunnel_length, MIN_SCALE) / std::sqrt(vmax(s.tunnels, MIN_SCALE));
}

float CaveCarver::sample(const std::array<float, Grid::N * Grid::N * Grid::N>& v, int lx, int ly, int lz) noexcept {
    const int x0 = lx / STEP, y0 = ly / STEP, z0 = lz / STEP;
    const float fx = static_cast<float>(lx % STEP) / STEP, fy = static_cast<float>(ly % STEP) / STEP, fz = static_cast<float>(lz % STEP) / STEP;
    auto at = [&](int dx, int dy, int dz) { return v[static_cast<std::size_t>(Grid::index(x0 + dx, y0 + dy, z0 + dz))]; };
    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    const float c00 = lerp(at(0, 0, 0), at(1, 0, 0), fx), c10 = lerp(at(0, 1, 0), at(1, 1, 0), fx);
    const float c01 = lerp(at(0, 0, 1), at(1, 0, 1), fx), c11 = lerp(at(0, 1, 1), at(1, 1, 1), fx);
    return lerp(lerp(c00, c10, fy), lerp(c01, c11, fy), fz);
}

} // namespace voxelspire
