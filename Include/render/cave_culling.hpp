#ifndef VOXELSPIRE_RENDER_CAVE_CULLING_HPP
#define VOXELSPIRE_RENDER_CAVE_CULLING_HPP

#include <cstdint>
#include <unordered_set>
#include <vector>
#include "../world/chunk_mesh.hpp"

namespace voxelspire {

class CaveCulling {
public:
    static constexpr std::uint8_t NO_FACE = 0xFF;

    template <typename ConnectivityFn>
    void compute(
        const ChunkPos& camera, const std::unordered_set<ColumnPos, ColumnPosHash>& region,
        int min_cz, int max_cz, ConnectivityFn&& connectivity
    ) {
        m_enabled = false;
        m_visited = 0;
        if (region.empty() || !region.count({ camera.x, camera.y }) || max_cz < min_cz) return;
        m_x0 = m_y0 = std::numeric_limits<int>::max();
        int x1 = std::numeric_limits<int>::min(), y1 = std::numeric_limits<int>::min();

        for (const ColumnPos& c : region) {
            m_x0 = vmin(m_x0, c.x); m_y0 = vmin(m_y0, c.y);
            x1 = vmax(x1, c.x);     y1 = vmax(y1, c.y);
        }

        m_w = x1 - m_x0 + 1;
        m_h = y1 - m_y0 + 1;
        m_z0 = min_cz;
        m_d = max_cz - min_cz + 1;
        const std::size_t n = static_cast<std::size_t>(m_w) * m_h * m_d;
        m_in_region.assign(static_cast<std::size_t>(m_w) * m_h, 0);
        for (const ColumnPos& c : region) m_in_region[static_cast<std::size_t>(c.y - m_y0) * m_w + (c.x - m_x0)] = 1;
        m_conn.assign(n, ChunkConnectivity::ALL);
        connectivity([this](const ChunkPos& p, const ChunkConnectivity& c) { if (inside(p)) m_conn[index(p)] = c.bits; });
        m_visible.assign(n, 0);
        const ChunkPos start{ camera.x, camera.y, vclamp(camera.z, min_cz, max_cz) };
        m_queue.clear();
        m_queue.push_back({ start, NO_FACE, 0 });
        m_visible[index(start)] = 1;
        m_visited = 1;

        for (std::size_t head = 0; head < m_queue.size(); ++head) {
            const Node cur = m_queue[head];
            const ChunkConnectivity conn{ m_conn[index(cur.pos)] };

            for (Face out : ALL_FACES) {
                const int bit = 1 << static_cast<int>(opposite(out));
                if (cur.dirs & bit) continue;
                if (cur.entered != NO_FACE && !conn.connected(static_cast<Face>(cur.entered), out)) continue;
                const BlockPos o = face_offset(out);
                const ChunkPos next{ cur.pos.x + o.x, cur.pos.y + o.y, cur.pos.z + o.z };
                if (!inside(next)) continue;
                const std::size_t ni = index(next);
                if (m_visible[ni]) continue;
                m_visible[ni] = 1;
                ++m_visited;
                m_queue.push_back({ next, static_cast<std::uint8_t>(opposite(out)), static_cast<std::uint8_t>(cur.dirs | (1 << static_cast<int>(out))) });
            }
        }

        m_enabled = true;
    }

    bool visible(const ChunkPos& p) const noexcept {
        if (!m_enabled) return true;
        if (!inside(p)) return true;
        return m_visible[index(p)] != 0;
    }

    bool        enabled() const noexcept { return m_enabled; }
    std::size_t visited() const noexcept { return m_visited; }

private:
    struct Node {
        ChunkPos     pos;
        std::uint8_t entered;
        std::uint8_t dirs;
    };

    bool inside(const ChunkPos& p) const noexcept;

    std::size_t index(const ChunkPos& p) const noexcept {
        return (static_cast<std::size_t>(p.z - m_z0) * m_h + (p.y - m_y0)) * m_w + (p.x - m_x0);
    }

    bool                       m_enabled = false;
    int                        m_x0 = 0, m_y0 = 0, m_z0 = 0, m_w = 0, m_h = 0, m_d = 0;
    std::size_t                m_visited = 0;
    std::vector<std::uint8_t>  m_in_region;
    std::vector<std::uint64_t> m_conn;
    std::vector<std::uint8_t>  m_visible;
    std::vector<Node>          m_queue;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_CAVE_CULLING_HPP