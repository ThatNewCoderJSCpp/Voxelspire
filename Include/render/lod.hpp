#ifndef VOXELSPIRE_RENDER_LOD_HPP
#define VOXELSPIRE_RENDER_LOD_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../world/world_generator.hpp"
#include "greedy_mesher.hpp"

namespace voxelspire {

struct LodTileKey {
    int level = 0, x = 0, y = 0;
    constexpr bool operator==(const LodTileKey& o) const noexcept { return level == o.level && x == o.x && y == o.y; }
    constexpr bool operator!=(const LodTileKey& o) const noexcept { return !(*this == o); }
};

struct LodTileKeyHash {
    std::size_t operator()(const LodTileKey& k) const noexcept {
        std::uint64_t h = static_cast<std::uint32_t>(k.x) * 0x9E3779B97F4A7C15ull;
        h ^= static_cast<std::uint32_t>(k.y) * 0xC2B2AE3D27D4EB4Full + (h << 6) + (h >> 2);
        h ^= static_cast<std::uint64_t>(k.level) * 0x165667B19E3779F9ull;
        return static_cast<std::size_t>(h);
    }
};

struct LodLayout {
    int cells = 32;

    int cell_size(int level)   const noexcept { return 1 << level; }
    int tile_blocks(int level) const noexcept { return cells << level; }

    BlockPos origin(const LodTileKey& k) const noexcept { return { k.x * tile_blocks(k.level), k.y * tile_blocks(k.level), 0 }; }

    ColumnPos first_column(const LodTileKey& k) const noexcept {
        const BlockPos o = origin(k);
        return { floor_div(o.x, Chunk::SIZE), floor_div(o.y, Chunk::SIZE) };
    }

    int columns_per_side(int level) const noexcept { return vmax(tile_blocks(level) / Chunk::SIZE, 1); }

    bool contains_column(const LodTileKey& k, const ColumnPos& c) const noexcept {
        const ColumnPos f = first_column(k);
        const int n = columns_per_side(k.level);
        return c.x >= f.x && c.x < f.x + n && c.y >= f.y && c.y < f.y + n;
    }

    LodTileKey parent_at(const ColumnPos& c, int level) const noexcept {
        const int tb = tile_blocks(level);
        return { level, floor_div(c.x * Chunk::SIZE, tb), floor_div(c.y * Chunk::SIZE, tb) };
    }
};

struct LodRun {
    std::int32_t  z0 = 0, z1 = 0;
    BlockId       id = AIR_ID;
    std::uint32_t count = 0;
};

struct LodBuildRequest {
    LodTileKey  key;
    LodLayout   layout;
    bool        heightmap = false;
    double      coverage  = 0.5;
    bool        merge     = true;
    bool        cull_void = true;
    int         samples   = 0;
    FaceShading shading;
    int         min_chunk_z = 0, max_chunk_z = 0;
    std::uint64_t epoch = 0;
    std::unordered_map<ChunkPos, std::shared_ptr<const Chunk>, ChunkPosHash> overrides;
};

struct LodTileMesh {
    LodTileKey                  key;
    std::uint64_t               epoch = 0;
    fizmo::graphics::QuadMesh3D quads;
    std::size_t                 faces = 0;
    int                         min_z = 0, max_z = 0;
};

class LodBuilder {
public:
    LodBuilder(const WorldGenerator& generator, const BlockRegistry& registry) : m_generator(generator), m_registry(registry) {}

    LodTileMesh build(const LodBuildRequest& req) {
        LodTileMesh out;
        out.key = req.key;
        out.epoch = req.epoch;
        const int s = req.layout.cell_size(req.key.level);
        const int C = req.layout.cells;
        const int G = C + 2;
        const BlockPos o = req.layout.origin(req.key);
        const int x0 = o.x - s, y0 = o.y - s;
        const int x1 = o.x + C * s + s, y1 = o.y + C * s + s;
        m_cells.assign(static_cast<std::size_t>(G) * G, {});
        std::vector<ColumnRun> probe;
        const bool sampled = req.samples > 0 && req.samples < s && m_generator.sample_column(o.x, o.y, probe);
        const int per_axis = sampled ? req.samples : s;
        if (sampled) gather_sampled(req, x0, y0, s, G);
        else gather(req, x0, y0, x1, y1, s, G);
        int zmin = std::numeric_limits<int>::max(), zmax = std::numeric_limits<int>::min();

        for (const auto& runs : m_cells)
            for (const LodRun& r : runs) { zmin = vmin(zmin, r.z0); zmax = vmax(zmax, r.z1); }

        if (zmin >= zmax) return out;
        const int Z = zmax - zmin;
        resolve(req, per_axis, G, Z, zmin);
        out.min_z = zmin;
        out.max_z = zmax;
        const int dims[3] = { C, C, Z };

        auto voxel = [&](int cx, int cy, int z) -> const Voxel* {
            if (z < 0 || z >= Z) return nullptr;
            return &m_voxels[(static_cast<std::size_t>(cy) * G + cx) * Z + z];
        };

        auto key_at = [&](Face f, int x, int y, int z) -> std::uint64_t {
            const Voxel* v = voxel(x + 1, y + 1, z);
            if (!v || !v->solid) return FaceKey::NONE;
            const BlockPos d = face_offset(f);
            const Voxel* n = voxel(x + 1 + d.x, y + 1 + d.y, z + d.z);
            if (n && n->solid) return FaceKey::NONE;
            if (f == Face::Down && req.cull_void && z == m_lowest[static_cast<std::size_t>(y + 1) * G + x + 1]) return FaceKey::NONE;
            const Color tint = f == Face::Up ? v->up : (f == Face::Down ? v->down : v->side);
            const Color c(tint.red(), tint.green(), tint.blue());
            return FaceKey::make(c, 0, req.shading.level(f), false, f == Face::Up ? v->finish : std::uint8_t(0));
        };

        const vector3d origin{ 0.0, 0.0, double(zmin) }, cell{ double(s), double(s), 1.0 };
        out.faces = m_greedy.mesh(dims, req.merge, key_at, [&](const GreedyRect& r) { emit_scaled_quad(out.quads, r, origin, cell); });
        out.quads.shrink_to_fit();
        return out;
    }

private:
    struct Voxel {
        bool          solid  = false;
        bool          opaque = false;
        std::uint8_t  finish = 0;
        Color         up, side, down;
    };

    struct Tally {
        BlockId       id;
        std::uint64_t r = 0, g = 0, b = 0;
    };

    void gather(const LodBuildRequest& req, int x0, int y0, int x1, int y1, int s, int G) {
        const int cx0 = floor_div(x0, Chunk::SIZE), cx1 = floor_div(x1 - 1, Chunk::SIZE);
        const int cy0 = floor_div(y0, Chunk::SIZE), cy1 = floor_div(y1 - 1, Chunk::SIZE);
        std::vector<std::shared_ptr<const Chunk>> column;

        for (int cy = cy0; cy <= cy1; ++cy) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                column = column_chunks(req, { cx, cy });

                if (column.empty()) continue;

                for (int ly = 0; ly < Chunk::SIZE; ++ly) {
                    const int wy = cy * Chunk::SIZE + ly;
                    if (wy < y0 || wy >= y1) continue;
                    const int gy = (wy - y0) / s;

                    for (int lx = 0; lx < Chunk::SIZE; ++lx) {
                        const int wx = cx * Chunk::SIZE + lx;
                        if (wx < x0 || wx >= x1) continue;
                        const int gx = (wx - x0) / s;
                        add_column(m_cells[static_cast<std::size_t>(gy) * G + gx], column, lx, ly);
                    }
                }
            }
        }
    }

    void gather_sampled(const LodBuildRequest& req, int x0, int y0, int s, int G) {
        const int n = req.samples;
        const int sub = vmax(s / n, 2);
        std::unordered_set<ColumnPos, ColumnPosHash> edited;
        for (const auto& kv : req.overrides) edited.insert({ kv.first.x, kv.first.y });
        std::unordered_map<ColumnPos, std::vector<std::shared_ptr<const Chunk>>, ColumnPosHash> exact;
        std::vector<ColumnRun> runs;

        for (int gy = 0; gy < G; ++gy)
            for (int gx = 0; gx < G; ++gx) {
                auto& cell = m_cells[static_cast<std::size_t>(gy) * G + gx];

                for (int sy = 0; sy < n; ++sy)
                    for (int sx = 0; sx < n; ++sx) {
                        const int stagger = (sx + sy) & 1;
                        const int wx = x0 + gx * s + sx * sub + sub / 2 - stagger;
                        const int wy = y0 + gy * s + sy * sub + sub / 2;
                        const ColumnPos col{ floor_div(wx, Chunk::SIZE), floor_div(wy, Chunk::SIZE) };

                        if (edited.count(col)) {
                            auto it = exact.find(col);
                            if (it == exact.end()) it = exact.emplace(col, column_chunks(req, col)).first;
                            if (!it->second.empty()) add_column(cell, it->second, floor_mod(wx, Chunk::SIZE), floor_mod(wy, Chunk::SIZE));
                            continue;
                        }

                        m_generator.sample_column(wx, wy, runs);
                        for (const ColumnRun& r : runs) add_run(cell, r.z0, r.z1, r.id);
                    }
            }
    }

    std::vector<std::shared_ptr<const Chunk>> column_chunks(const LodBuildRequest& req, const ColumnPos& col) const {
        std::vector<std::shared_ptr<const Chunk>> out;

        for (int cz = req.min_chunk_z; cz <= req.max_chunk_z; ++cz) {
            const ChunkPos p{ col.x, col.y, cz };
            auto it = req.overrides.find(p);
            if (it != req.overrides.end()) { if (it->second && !it->second->empty()) out.push_back(it->second); continue; }
            if (auto c = m_generator.generate_chunk(p)) out.push_back(std::shared_ptr<const Chunk>(std::move(c)));
        }

        return out;
    }

    static void add_run(std::vector<LodRun>& runs, int z0, int z1, BlockId id) {
        for (LodRun& r : runs) if (r.z0 == z0 && r.z1 == z1 && r.id == id) { ++r.count; return; }
        runs.push_back({ z0, z1, id, 1 });
    }

    static void add_column(std::vector<LodRun>& runs, const std::vector<std::shared_ptr<const Chunk>>& column, int lx, int ly) {
        BlockId cur = AIR_ID;
        int start = 0, last_z = std::numeric_limits<int>::min();

        for (const auto& c : column) {
            const int base = c->pos().z * Chunk::SIZE;

            for (int lz = 0; lz < Chunk::SIZE; ++lz) {
                const int z = base + lz;
                const BlockId id = c->get(lx, ly, lz);
                const bool gap = z != last_z + 1;

                if (cur != AIR_ID && (id != cur || gap)) { add_run(runs, start, last_z + 1, cur); cur = AIR_ID; }
                if (id != AIR_ID && id != cur) { cur = id; start = z; }
                last_z = z;
            }
        }

        if (cur != AIR_ID) add_run(runs, start, last_z + 1, cur);
    }

    void resolve(const LodBuildRequest& req, int per_axis, int G, int Z, int zmin) {
        const double needed = req.coverage * static_cast<double>(per_axis) * static_cast<double>(per_axis);
        m_voxels.assign(static_cast<std::size_t>(G) * G * Z, Voxel{});
        m_lowest.assign(static_cast<std::size_t>(G) * G, std::numeric_limits<int>::max());
        std::vector<BlockId> ids;
        std::vector<std::uint32_t> counts;

        for (int cell = 0; cell < G * G; ++cell) {
            const auto& runs = m_cells[static_cast<std::size_t>(cell)];
            if (runs.empty()) continue;
            ids.clear();
            for (const LodRun& r : runs) if (std::find(ids.begin(), ids.end(), r.id) == ids.end()) ids.push_back(r.id);
            const std::size_t K = ids.size();
            counts.assign(K * static_cast<std::size_t>(Z), 0);

            for (const LodRun& r : runs) {
                const std::size_t k = static_cast<std::size_t>(std::find(ids.begin(), ids.end(), r.id) - ids.begin());
                for (int z = r.z0; z < r.z1; ++z) counts[k * Z + static_cast<std::size_t>(z - zmin)] += r.count;
            }

            Voxel* col = &m_voxels[static_cast<std::size_t>(cell) * Z];
            int top = -1;

            for (int z = 0; z < Z; ++z) {
                std::uint64_t total = 0, best = 0;
                std::size_t best_k = 0;

                for (std::size_t k = 0; k < K; ++k) {
                    const std::uint32_t n = counts[k * Z + static_cast<std::size_t>(z)];
                    total += n;
                    if (n > best) { best = n; best_k = k; }
                }

                if (total == 0) continue;
                const bool solid = static_cast<double>(total) >= needed;
                if (!solid && !req.heightmap) continue;
                Voxel& v = col[z];
                v.solid  = solid;
                v.opaque = m_registry.get(ids[best_k]).is_opaque();
                v.finish = FaceKey::finish_flags(m_registry.traits(ids[best_k]).finish);
                v.up   = average(ids, counts, K, Z, z, Face::Up);
                v.side = average(ids, counts, K, Z, z, Face::East);
                v.down = average(ids, counts, K, Z, z, Face::Down);
                if (solid) top = z;
            }

            if (req.heightmap) {
                if (top < 0) { for (int z = 0; z < Z; ++z) col[z] = Voxel{}; continue; }
                Voxel carry = col[top];

                for (int z = Z - 1; z >= 0; --z) {
                    if (z > top) { col[z] = Voxel{}; continue; }
                    if (col[z].side.alpha() != 0 || col[z].up.alpha() != 0) carry = col[z];
                    col[z] = carry;
                    col[z].solid = true;
                }
            }

            for (int z = 0; z < Z; ++z) if (col[z].solid) { m_lowest[static_cast<std::size_t>(cell)] = z; break; }
        }
    }

    Color average(const std::vector<BlockId>& ids, const std::vector<std::uint32_t>& counts, std::size_t K, int Z, int z, Face f) const {
        std::uint64_t r = 0, g = 0, b = 0, a = 0, n = 0;

        for (std::size_t k = 0; k < K; ++k) {
            const std::uint32_t c = counts[k * Z + static_cast<std::size_t>(z)];
            if (c == 0) continue;
            const Block& block = m_registry.get(ids[k]);
            if (!block.is_visible()) continue;
            const Color col = block.face_appearance(f).base;
            r += std::uint64_t(col.red()) * c; g += std::uint64_t(col.green()) * c; b += std::uint64_t(col.blue()) * c; a += std::uint64_t(col.alpha()) * c;
            n += c;
        }

        if (n == 0) return Color(0, 0, 0, 0);

        return Color(
            static_cast<std::uint8_t>((r + n / 2) / n), 
            static_cast<std::uint8_t>((g + n / 2) / n),
            static_cast<std::uint8_t>((b + n / 2) / n), 
            static_cast<std::uint8_t>((a + n / 2) / n)
        );
    }

    const WorldGenerator&            m_generator;
    const BlockRegistry&             m_registry;
    GreedyMesher                     m_greedy;
    std::vector<std::vector<LodRun>> m_cells;
    std::vector<Voxel>               m_voxels;
    std::vector<int>                 m_lowest;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_LOD_HPP