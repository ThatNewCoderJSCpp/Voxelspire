#ifndef VOXELSPIRE_RENDER_TERRAIN_MESHES_HPP
#define VOXELSPIRE_RENDER_TERRAIN_MESHES_HPP

#include <algorithm>
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../core/job_system.hpp"
#include "../world/chunk_mesh.hpp"
#include "../world/chunk_streamer.hpp"
#include "cave_culling.hpp"
#include "lod_selection.hpp"

namespace voxelspire {

struct ChunkRenderEntry {
    fizmo::graphics::MeshHandle3D  opaque;
    fizmo::graphics::MeshHandle3D  translucent;
    std::unique_ptr<ChunkMeshData> pending;
    ChunkConnectivity              connectivity;
    std::size_t                    faces          = 0, quads = 0, translucent_quads = 0, cpu_bytes = 0;
    std::uint64_t                  built_revision = 0;
    std::uint64_t                  opaque_hash    = 0;
    bool                           meshed         = false;
    bool                           sunlit         = true;
    bool                           fluid_tops     = false;
    bool                           split          = false;
    bool                           in_flight      = false;
};

struct LodTileEntry {
    fizmo::graphics::MeshHandle3D handle;
    std::unique_ptr<LodTileMesh>  pending;
    std::size_t                   faces = 0, quads = 0, cpu_bytes = 0;
    int                           min_z = 0, max_z = 0;
    std::uint64_t                 epoch = 1;
    std::uint64_t                 built_epoch = 0;
    bool                          ready = false;
    bool                          in_flight = false;
    bool                          selected = true;
};

struct TerrainStats {
    std::size_t   chunk_meshes      = 0;
    std::size_t   chunk_quads       = 0;
    std::size_t   translucent_quads = 0;
    std::size_t   chunk_faces       = 0;
    std::size_t   lod_tiles         = 0;
    std::size_t   lod_tiles_ready   = 0;
    std::size_t   lod_quads         = 0;
    std::size_t   retiring_tiles    = 0;
    std::size_t   mesh_jobs         = 0;
    std::size_t   lod_jobs          = 0;
    std::size_t   waiting_meshes    = 0;
    std::size_t   pending_uploads   = 0;
    std::size_t   cpu_mesh_bytes    = 0;
    std::uint64_t gpu_mesh_bytes    = 0;
    std::size_t   cave_visited      = 0;
    std::size_t   uploads           = 0;
    std::size_t   reused            = 0;
};

struct TerrainColumn {
    std::vector<std::pair<int, ChunkRenderEntry*>> chunks;
    int min_z = 0, max_z = 0;

    void refresh_bounds() noexcept;
};

class TerrainMeshes {
public:
    using ChunkMap  = std::unordered_map<ChunkPos, ChunkRenderEntry, ChunkPosHash>;
    using TileMap   = std::unordered_map<LodTileKey, LodTileEntry, LodTileKeyHash>;
    using ColumnSet = std::unordered_set<ColumnPos, ColumnPosHash>;

    TerrainMeshes(World& world, const WorldGenerator& generator, JobSystem& jobs, ChunkStreamer& streamer)
        : m_world(world), m_generator(generator), m_jobs(jobs), m_streamer(streamer) {}

    void configure(const RenderSettings& render, const StreamingSettings& streaming, const LodSettings& lod, const LightingSettings& lighting);

    const LodLayout&    layout()    const noexcept { return m_layout; }
    const LodSelection& selection() const noexcept { return m_selection; }
    const ChunkMap&     chunks()    const noexcept { return m_chunks; }
    const TileMap&      tiles()     const noexcept { return m_tiles; }

    void set_wave_detail(int chunks) {
        if (chunks == m_wave_detail) return;
        m_wave_detail = chunks;
        m_wave_dirty = true;
    }

    int wave_detail() const noexcept { return m_wave_detail; }

    void set_water(WaterLookPtr water, bool waves);

    void remesh_all();

    void set_selection(LodSelection selection);

    void update(const vector3d& camera);

    void upload(fizmo::windows::Renderer& renderer);

    void mark_lost(const ChunkPos& p);

    void mark_lost(const LodTileKey& k);

    const CaveCulling& cave_culling(const ChunkPos& camera_chunk, bool enabled);

    template <typename Fn>
    void for_each_chunk_in(const ColumnPos& c, Fn&& fn) const {
        auto col = m_columns.find(c);
        if (col == m_columns.end()) return;
        for (const auto& z : col->second.chunks) fn(ChunkPos{ c.x, c.y, z.first }, *z.second);
    }

    template <typename Fn>
    void for_each_drawn_column(Fn&& fn) const {
        auto visit = [&](const ColumnPos& c) {
            if (!m_suppressed_columns.empty() && m_suppressed_columns.count(c)) return;
            auto col = m_columns.find(c);
            if (col != m_columns.end()) fn(c, col->second);
        };

        for (const ColumnPos& c : m_selection.detail_columns) visit(c);
        for (const auto& kv : m_retiring_columns) visit(kv.first);
    }

    template <typename Fn>
    void for_each_drawn_tile(Fn&& fn) const {
        for (const LodTileKey& k : m_selection.tiles) {
            if (m_suppressed_tiles.count(k)) continue;
            auto it = m_tiles.find(k);
            if (it != m_tiles.end() && it->second.ready) fn(k, it->second);
        }

        for (const auto& kv : m_retiring_tiles) {
            auto it = m_tiles.find(kv.first);
            if (it != m_tiles.end() && it->second.ready) fn(kv.first, it->second);
        }
    }

    TerrainStats stats() const;

private:
    struct TileTransition {
        std::vector<LodTileKey> tiles;
        std::vector<ColumnPos>  columns;
    };

    void count(const ChunkRenderEntry& e, int sign = 1) noexcept;

    void uncount(const ChunkRenderEntry& e) noexcept { count(e, -1); }

    void count(const LodTileEntry& t, int sign = 1) noexcept;

    void uncount(const LodTileEntry& t) noexcept { count(t, -1); }

    ChunkRenderEntry& entry(const ChunkPos& p);

    void erase_chunk(const ChunkPos& p);

    void want(const ChunkPos& p) { m_wanted.insert(p); }

    void enqueue_tile(const LodTileKey& k);

    void handle_column_events();

    void handle_changes();

    bool neighbors_loaded(const ColumnPos& c) const;

    ColumnPos camera_column() const noexcept;

    bool wave_detailed(const ChunkPos& p) const noexcept;

    void refresh_waves();

    double chunk_distance_sq(const ChunkPos& p) const noexcept;

    void schedule_meshes();

    void submit_mesh(const ChunkPos& p);

    void on_meshed(std::unique_ptr<ChunkMeshData> data);

    double tile_distance(const LodTileKey& k) const noexcept;

    void schedule_tiles();

    void submit_tile(const LodTileKey& k, LodTileEntry& t);

    void on_tile_built(std::unique_ptr<LodTileMesh> mesh);

    bool column_ready(const ColumnPos& c) const;

    bool tile_ready(const LodTileKey& k) const {
        auto it = m_tiles.find(k);
        return it != m_tiles.end() && it->second.ready;
    }

    bool tile_overlaps(const LodTileKey& a, const LodTileKey& b) const noexcept;

    void plan_transitions();

    void resolve_transitions();

    World&                m_world;
    const WorldGenerator& m_generator;
    JobSystem&            m_jobs;
    ChunkStreamer&        m_streamer;
    ChunkMeshOptions      m_mesh_options;
    StreamingSettings     m_streaming;
    LodSettings           m_lod;
    LodLayout             m_layout;
    LodSelection          m_selection;
    vector3d              m_camera{};
    ColumnPos             m_wave_center{};
    int                   m_wave_detail = 2;
    bool                  m_wave_dirty = true;

    ChunkMap              m_chunks;
    TileMap               m_tiles;
    std::unordered_map<ColumnPos, TerrainColumn, ColumnPosHash> m_columns;
    std::unordered_set<ChunkPos, ChunkPosHash>      m_wanted;
    std::unordered_set<LodTileKey, LodTileKeyHash>  m_tile_queue;
    std::vector<ChunkPos>   m_candidates;
    std::vector<LodTileKey> m_tile_candidates;
    std::deque<ChunkPos>    m_upload_order;
    std::deque<LodTileKey>  m_tile_upload_order;

    std::unordered_map<LodTileKey, TileTransition, LodTileKeyHash> m_retiring_tiles;
    std::unordered_map<ColumnPos, LodTileKey, ColumnPosHash>       m_retiring_columns;
    std::unordered_set<ColumnPos, ColumnPosHash>                   m_suppressed_columns;
    std::unordered_set<LodTileKey, LodTileKeyHash>                 m_suppressed_tiles;

    TerrainStats            m_totals;
    int                     m_mesh_jobs = 0;
    int                     m_tile_jobs = 0;
    std::size_t             m_pending_uploads = 0;
    std::size_t             m_pending_bytes = 0;
    std::uint64_t           m_gpu_bytes = 0;
    std::size_t             m_uploads = 0;
    std::size_t             m_reused = 0;

    CaveCulling             m_cave;
    ChunkPos                m_cave_chunk{ 0, 0, std::numeric_limits<int>::min() };
    bool                    m_cave_dirty = true;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_TERRAIN_MESHES_HPP