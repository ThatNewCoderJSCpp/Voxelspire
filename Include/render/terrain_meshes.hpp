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
    std::size_t                    faces = 0, quads = 0, translucent_quads = 0, cpu_bytes = 0;
    std::uint64_t                  built_revision = 0;
    bool                           meshed = false;
    bool                           in_flight = false;
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
};

struct TerrainColumn {
    std::vector<std::pair<int, ChunkRenderEntry*>> chunks;
    int min_z = 0, max_z = 0;

    void refresh_bounds() noexcept {
        if (chunks.empty()) return;
        min_z = max_z = chunks.front().first;
        for (const auto& c : chunks) { min_z = vmin(min_z, c.first); max_z = vmax(max_z, c.first); }
    }
};

class TerrainMeshes {
public:
    using ChunkMap  = std::unordered_map<ChunkPos, ChunkRenderEntry, ChunkPosHash>;
    using TileMap   = std::unordered_map<LodTileKey, LodTileEntry, LodTileKeyHash>;
    using ColumnSet = std::unordered_set<ColumnPos, ColumnPosHash>;

    TerrainMeshes(World& world, const WorldGenerator& generator, JobSystem& jobs, ChunkStreamer& streamer)
        : m_world(world), m_generator(generator), m_jobs(jobs), m_streamer(streamer) {}

    void configure(const RenderSettings& render, const StreamingSettings& streaming, const LodSettings& lod, const LightingSettings& lighting) {
        ChunkMeshOptions next;
        next.merge_faces       = render.merge_faces;
        next.cull_void_faces   = render.cull_void_faces;
        next.smooth_lighting   = lighting.baked_light && lighting.smooth_lighting;
        next.ambient_occlusion = lighting.ambient_occlusion;
        next.occlusion_step    = lighting.occlusion_step;
        next.shading.factors   = render.shading;
        next.shading.strength  = lighting.face_shading;
        const bool changed = next != m_mesh_options;
        m_mesh_options = next;
        m_streaming = streaming;
        m_lod = lod;
        m_layout.cells = vmax(lod.tile_cells - lod.tile_cells % Chunk::SIZE, Chunk::SIZE);
        if (changed) remesh_all();
    }

    const LodLayout&    layout()    const noexcept { return m_layout; }
    const LodSelection& selection() const noexcept { return m_selection; }
    const ChunkMap&     chunks()    const noexcept { return m_chunks; }
    const TileMap&      tiles()     const noexcept { return m_tiles; }

    void remesh_all() {
        for (const auto& kv : m_chunks) want(kv.first);
        for (auto& kv : m_tiles) { ++kv.second.epoch; enqueue_tile(kv.first); }
    }

    void set_selection(LodSelection selection) {
        std::unordered_set<LodTileKey, LodTileKeyHash> keep(selection.tiles.begin(), selection.tiles.end());

        for (auto it = m_tiles.begin(); it != m_tiles.end();) {
            LodTileEntry& t = it->second;
            const bool now = keep.count(it->first) != 0;
            if (!now && !t.ready) { m_tile_queue.erase(it->first); it = m_tiles.erase(it); continue; }
            t.selected = now;
            if (now) m_retiring_tiles.erase(it->first); else m_retiring_tiles[it->first];
            ++it;
        }

        for (const LodTileKey& k : selection.tiles) {
            m_tiles[k].selected = true;
            enqueue_tile(k);
        }

        for (const ColumnPos& c : m_selection.detail_columns)
            if (!selection.detail_set.count(c)) m_retiring_columns[c];

        for (const ColumnPos& c : selection.detail_columns) m_retiring_columns.erase(c);
        m_selection = std::move(selection);
        plan_transitions();
        m_cave_dirty = true;
    }

    void update(const vector3d& camera) {
        m_camera = camera;
        handle_column_events();
        handle_changes();
        schedule_meshes();
        schedule_tiles();
        resolve_transitions();
    }

    void upload(fizmo::windows::Renderer& renderer) {
        const std::size_t budget = m_streaming.upload_bytes_per_frame;
        const bool keeps_ram = !renderer.is_gpu();
        std::size_t used = 0;

        while (!m_upload_order.empty() && used < budget) {
            auto e = m_chunks.find(m_upload_order.front());
            m_upload_order.pop_front();
            if (e == m_chunks.end() || !e->second.pending) continue;
            ChunkRenderEntry& entry = e->second;
            std::unique_ptr<ChunkMeshData> data = std::move(entry.pending);
            m_pending_bytes -= data->quads.memory_bytes() + data->translucent.memory_bytes();
            --m_pending_uploads;
            used += data->quads.memory_bytes() + data->translucent.memory_bytes();
            uncount(entry);
            const ChunkConnectivity old = entry.connectivity;
            entry.connectivity      = data->connectivity;
            entry.faces             = data->faces;
            entry.quads             = data->quads.quad_count();
            entry.translucent_quads = data->translucent.quad_count();
            entry.cpu_bytes         = keeps_ram ? data->quads.memory_bytes() + data->translucent.memory_bytes() : 0;
            entry.built_revision    = data->revision;
            entry.opaque      = data->quads.empty()       ? fizmo::graphics::MeshHandle3D() : renderer.upload_quads(std::move(data->quads));
            entry.translucent = data->translucent.empty() ? fizmo::graphics::MeshHandle3D() : renderer.upload_quads(std::move(data->translucent));
            entry.meshed = true;
            count(entry);
            if (old.bits != entry.connectivity.bits) m_cave_dirty = true;
        }

        while (!m_tile_upload_order.empty() && used < budget) {
            auto e = m_tiles.find(m_tile_upload_order.front());
            m_tile_upload_order.pop_front();
            if (e == m_tiles.end() || !e->second.pending) continue;
            LodTileEntry& t = e->second;
            std::unique_ptr<LodTileMesh> data = std::move(t.pending);
            m_pending_bytes -= data->quads.memory_bytes();
            --m_pending_uploads;
            used += data->quads.memory_bytes();
            uncount(t);
            t.faces       = data->faces;
            t.quads       = data->quads.quad_count();
            t.cpu_bytes   = keeps_ram ? data->quads.memory_bytes() : 0;
            t.min_z       = data->min_z;
            t.max_z       = data->max_z;
            t.built_epoch = data->epoch;
            t.handle      = data->quads.empty() ? fizmo::graphics::MeshHandle3D() : renderer.upload_quads(std::move(data->quads));
            t.ready       = true;
            count(t);
        }

        m_gpu_bytes = renderer.gpu_mesh_bytes();
    }

    void mark_lost(const ChunkPos& p) {
        auto it = m_chunks.find(p);
        if (it == m_chunks.end()) return;
        uncount(it->second);
        it->second.opaque.reset();
        it->second.translucent.reset();
        it->second.meshed = false;
        count(it->second);
        want(p);
    }

    void mark_lost(const LodTileKey& k) {
        auto it = m_tiles.find(k);
        if (it == m_tiles.end()) return;
        uncount(it->second);
        it->second.handle.reset();
        it->second.ready = false;
        ++it->second.epoch;
        count(it->second);
        enqueue_tile(k);
    }

    const CaveCulling& cave_culling(const ChunkPos& camera_chunk, bool enabled) {
        if (!enabled) { m_cave = CaveCulling(); m_cave_dirty = true; return m_cave; }

        if (m_cave_dirty || camera_chunk != m_cave_chunk) {
            m_cave.compute(camera_chunk, m_selection.detail_set, m_world.min_chunk_z(), m_world.max_chunk_z(), [this](auto&& put) {
                for (const ColumnPos& c : m_selection.detail_columns)
                    for_each_chunk_in(c, [&](const ChunkPos& p, const ChunkRenderEntry& e) { if (e.meshed) put(p, e.connectivity); });
            });
            m_cave_chunk = camera_chunk;
            m_cave_dirty = false;
        }

        return m_cave;
    }

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

    TerrainStats stats() const {
        TerrainStats s = m_totals;
        s.lod_tiles       = m_selection.tiles.size();
        s.retiring_tiles  = m_retiring_tiles.size();
        s.mesh_jobs       = static_cast<std::size_t>(m_mesh_jobs);
        s.lod_jobs        = static_cast<std::size_t>(m_tile_jobs);
        s.waiting_meshes  = m_wanted.size();
        s.pending_uploads = m_pending_uploads;
        s.cpu_mesh_bytes += m_pending_bytes;
        s.gpu_mesh_bytes  = m_gpu_bytes;
        s.cave_visited    = m_cave.visited();
        return s;
    }

private:
    struct TileTransition {
        std::vector<LodTileKey> tiles;
        std::vector<ColumnPos>  columns;
    };

    void count(const ChunkRenderEntry& e, int sign = 1) noexcept {
        auto add = [sign](std::size_t& total, std::size_t v) { total = sign > 0 ? total + v : total - v; };
        if (e.meshed && e.quads + e.translucent_quads > 0) add(m_totals.chunk_meshes, 1);
        add(m_totals.chunk_quads, e.quads);
        add(m_totals.translucent_quads, e.translucent_quads);
        add(m_totals.chunk_faces, e.faces);
        add(m_totals.cpu_mesh_bytes, e.cpu_bytes);
    }

    void uncount(const ChunkRenderEntry& e) noexcept { count(e, -1); }

    void count(const LodTileEntry& t, int sign = 1) noexcept {
        auto add = [sign](std::size_t& total, std::size_t v) { total = sign > 0 ? total + v : total - v; };
        if (t.ready) add(m_totals.lod_tiles_ready, 1);
        add(m_totals.lod_quads, t.quads);
        add(m_totals.cpu_mesh_bytes, t.cpu_bytes);
    }

    void uncount(const LodTileEntry& t) noexcept { count(t, -1); }

    ChunkRenderEntry& entry(const ChunkPos& p) {
        auto it = m_chunks.find(p);
        if (it != m_chunks.end()) return it->second;
        ChunkRenderEntry& e = m_chunks[p];
        TerrainColumn& col = m_columns[{ p.x, p.y }];
        col.chunks.emplace_back(p.z, &e);
        col.refresh_bounds();
        return e;
    }

    void erase_chunk(const ChunkPos& p) {
        auto it = m_chunks.find(p);
        if (it == m_chunks.end()) return;
        uncount(it->second);

        if (it->second.pending) {
            m_pending_bytes -= it->second.pending->quads.memory_bytes() + it->second.pending->translucent.memory_bytes();
            --m_pending_uploads;
        }

        m_chunks.erase(it);
        auto col = m_columns.find({ p.x, p.y });

        if (col != m_columns.end()) {
            auto& zs = col->second.chunks;
            zs.erase(std::remove_if(zs.begin(), zs.end(), [&](const auto& c) { return c.first == p.z; }), zs.end());
            if (zs.empty()) m_columns.erase(col);
            else col->second.refresh_bounds();
        }
    }

    void want(const ChunkPos& p) { m_wanted.insert(p); }

    void enqueue_tile(const LodTileKey& k) {
        auto it = m_tiles.find(k);
        if (it == m_tiles.end()) return;
        const LodTileEntry& t = it->second;
        if (!t.selected || t.in_flight || t.built_epoch == t.epoch) return;
        if (t.pending && t.pending->epoch == t.epoch) return;
        m_tile_queue.insert(k);
    }

    void handle_column_events() {
        for (const auto& ev : m_world.take_column_events()) {
            if (ev.second != ColumnEvent::Unloaded) continue;
            const ColumnPos& c = ev.first;
            auto col = m_columns.find(c);

            if (col != m_columns.end()) {
                std::vector<int> zs;
                for (const auto& z : col->second.chunks) zs.push_back(z.first);
                for (int z : zs) { erase_chunk({ c.x, c.y, z }); m_wanted.erase({ c.x, c.y, z }); }
            }

            m_retiring_columns.erase(c);
            m_cave_dirty = true;
        }
    }

    void handle_changes() {
        const auto changed = m_world.take_changed();
        m_streamer.note_changes(changed);
        for (const ChunkPos& p : changed) want(p);

        for (const ColumnPos& c : m_world.take_edited_columns()) {
            for (int level = 1; level <= m_lod.max_level; ++level) {
                const LodTileKey k = m_layout.parent_at(c, level);
                auto it = m_tiles.find(k);
                if (it == m_tiles.end()) continue;
                ++it->second.epoch;
                enqueue_tile(k);
            }
        }
    }

    bool neighbors_loaded(const ColumnPos& c) const {
        static constexpr int DX[4] = { -1, 1, 0, 0 }, DY[4] = { 0, 0, -1, 1 };

        for (int i = 0; i < 4; ++i) {
            const ColumnPos n{ c.x + DX[i], c.y + DY[i] };
            if (!m_world.column_loaded(n) && m_world.column_in_bounds(n)) return false;
        }

        return true;
    }

    double chunk_distance_sq(const ChunkPos& p) const noexcept {
        const double dx = (p.x + 0.5) * Chunk::SIZE - m_camera.x;
        const double dy = (p.y + 0.5) * Chunk::SIZE - m_camera.y;
        const double dz = (p.z + 0.5) * Chunk::SIZE - m_camera.z;
        return dx * dx + dy * dy + dz * dz;
    }

    void schedule_meshes() {
        const int room = m_streaming.max_mesh_jobs - m_mesh_jobs;
        if (room <= 0 || m_wanted.empty()) return;
        m_candidates.clear();

        for (auto it = m_wanted.begin(); it != m_wanted.end();) {
            const ChunkPos p = *it;

            if (!m_world.chunk_at(p)) {
                erase_chunk(p);
                it = m_wanted.erase(it);
                continue;
            }

            auto e = m_chunks.find(p);
            if ((e == m_chunks.end() || !e->second.in_flight) && neighbors_loaded({ p.x, p.y }) && m_world.light_settled({ p.x, p.y })) m_candidates.push_back(p);
            ++it;
        }

        const std::size_t count = vmin(static_cast<std::size_t>(room), m_candidates.size());

        std::partial_sort(m_candidates.begin(), m_candidates.begin() + static_cast<std::ptrdiff_t>(count), m_candidates.end(), [&](const ChunkPos& a, const ChunkPos& b) {
            const bool da = m_selection.detail_set.count({ a.x, a.y }) != 0, db = m_selection.detail_set.count({ b.x, b.y }) != 0;
            if (da != db) return da;
            return chunk_distance_sq(a) < chunk_distance_sq(b);
        });

        for (std::size_t i = 0; i < count; ++i) submit_mesh(m_candidates[i]);
    }

    void submit_mesh(const ChunkPos& p) {
        const Chunk* chunk = m_world.chunk_at(p);
        m_wanted.erase(p);
        entry(p).in_flight = true;
        ++m_mesh_jobs;
        auto snap = ChunkSnapshot::capture(m_world, *chunk);
        const BlockRegistry* reg = &m_world.blocks();
        const ChunkMeshOptions options = m_mesh_options;
        JobSystem* jobs = &m_jobs;

        m_jobs.submit([this, jobs, reg, options, snap = std::move(snap)]() mutable {
            ChunkMesher mesher(*reg);
            auto data = std::make_unique<ChunkMeshData>(mesher.build(*snap, options));
            jobs->post([this, data = std::move(data)]() mutable { on_meshed(std::move(data)); });
        });
    }

    void on_meshed(std::unique_ptr<ChunkMeshData> data) {
        --m_mesh_jobs;
        auto it = m_chunks.find(data->pos);
        if (it == m_chunks.end()) return;
        ChunkRenderEntry& e = it->second;
        e.in_flight = false;
        const Chunk* chunk = m_world.chunk_at(data->pos);
        if (!chunk) { erase_chunk(data->pos); return; }
        if (chunk->revision() != data->revision) { want(data->pos); return; }

        if (e.pending) {
            m_pending_bytes -= e.pending->quads.memory_bytes() + e.pending->translucent.memory_bytes();
        } else {
            m_upload_order.push_back(data->pos);
            ++m_pending_uploads;
        }

        m_pending_bytes += data->quads.memory_bytes() + data->translucent.memory_bytes();
        e.pending = std::move(data);
    }

    double tile_distance(const LodTileKey& k) const noexcept {
        const BlockPos o = m_layout.origin(k);
        const double size = m_layout.tile_blocks(k.level);
        return LodSelector::distance_to_rect(m_camera.x, m_camera.y, o.x, o.y, o.x + size, o.y + size);
    }

    void schedule_tiles() {
        if (!m_lod.enabled || m_tile_queue.empty()) return;
        const int room = static_cast<int>(m_lod.max_tile_jobs) - m_tile_jobs;
        if (room <= 0) return;
        m_tile_candidates.assign(m_tile_queue.begin(), m_tile_queue.end());
        const std::size_t count = vmin(static_cast<std::size_t>(room), m_tile_candidates.size());

        std::partial_sort(
            m_tile_candidates.begin(), 
            m_tile_candidates.begin() + static_cast<std::ptrdiff_t>(count), 
            m_tile_candidates.end(),
            [&](const LodTileKey& a, const LodTileKey& b) { 
                return tile_distance(a) < tile_distance(b); 
            }
        );

        for (std::size_t i = 0; i < count; ++i) {
            const LodTileKey k = m_tile_candidates[i];
            m_tile_queue.erase(k);
            auto it = m_tiles.find(k);
            if (it == m_tiles.end() || !it->second.selected) continue;
            submit_tile(k, it->second);
        }
    }

    void submit_tile(const LodTileKey& k, LodTileEntry& t) {
        t.in_flight = true;
        ++m_tile_jobs;
        LodBuildRequest req;
        req.key         = k;
        req.layout      = m_layout;
        req.heightmap   = k.level >= m_lod.heightmap_level;
        req.coverage    = m_lod.coverage_threshold;
        req.merge       = m_mesh_options.merge_faces;
        req.cull_void   = m_mesh_options.cull_void_faces;
        req.samples     = k.level > m_lod.exact_levels ? vmax(m_lod.samples_per_cell, 1) : 0;
        req.shading     = m_mesh_options.shading;
        req.min_chunk_z = m_world.min_chunk_z();
        req.max_chunk_z = m_world.max_chunk_z();
        req.epoch       = t.epoch;
        const BlockPos o = m_layout.origin(k);
        const int s = m_layout.cell_size(k.level), tb = m_layout.tile_blocks(k.level);
        req.overrides = m_streamer.collect_overrides(o.x - s, o.y - s, o.x + tb + s, o.y + tb + s);
        const WorldGenerator* gen = &m_generator;
        const BlockRegistry* reg = &m_world.blocks();
        JobSystem* jobs = &m_jobs;

        m_jobs.submit([this, jobs, gen, reg, req = std::move(req)]() mutable {
            LodBuilder builder(*gen, *reg);
            auto mesh = std::make_unique<LodTileMesh>(builder.build(req));
            jobs->post([this, mesh = std::move(mesh)]() mutable { on_tile_built(std::move(mesh)); });
        });
    }

    void on_tile_built(std::unique_ptr<LodTileMesh> mesh) {
        --m_tile_jobs;
        auto it = m_tiles.find(mesh->key);
        if (it == m_tiles.end()) return;
        LodTileEntry& t = it->second;
        t.in_flight = false;
        if (mesh->epoch != t.epoch) { enqueue_tile(mesh->key); return; }

        if (t.pending) {
            m_pending_bytes -= t.pending->quads.memory_bytes();
        } else {
            m_tile_upload_order.push_back(mesh->key);
            ++m_pending_uploads;
        }

        m_pending_bytes += mesh->quads.memory_bytes();
        t.pending = std::move(mesh);
    }

    bool column_ready(const ColumnPos& c) const {
        const ChunkColumn* col = m_world.column(c);
        if (!col) return !m_world.column_in_bounds(c);

        for (int z : col->chunk_zs) {
            auto it = m_chunks.find({ c.x, c.y, z });
            if (it == m_chunks.end() || !it->second.meshed) return false;
        }

        return true;
    }

    bool tile_ready(const LodTileKey& k) const {
        auto it = m_tiles.find(k);
        return it != m_tiles.end() && it->second.ready;
    }

    bool tile_overlaps(const LodTileKey& a, const LodTileKey& b) const noexcept {
        const BlockPos oa = m_layout.origin(a), ob = m_layout.origin(b);
        const int sa = m_layout.tile_blocks(a.level), sb = m_layout.tile_blocks(b.level);
        return oa.x < ob.x + sb && ob.x < oa.x + sa && oa.y < ob.y + sb && ob.y < oa.y + sa;
    }

    void plan_transitions() {
        for (auto& kv : m_retiring_tiles) {
            TileTransition& tr = kv.second;
            tr.tiles.clear();
            tr.columns.clear();
            for (const LodTileKey& k : m_selection.tiles) if (tile_overlaps(kv.first, k)) tr.tiles.push_back(k);
            for (const ColumnPos& c : m_selection.detail_columns) if (m_layout.contains_column(kv.first, c)) tr.columns.push_back(c);
        }

        for (auto it = m_retiring_columns.begin(); it != m_retiring_columns.end();) {
            bool covered = false;

            for (const LodTileKey& k : m_selection.tiles) {
                if (!m_layout.contains_column(k, it->first)) continue;
                it->second = k;
                covered = true;
                break;
            }

            if (covered && m_world.column_loaded(it->first)) ++it; else it = m_retiring_columns.erase(it);
        }
    }

    void resolve_transitions() {
        m_suppressed_tiles.clear();
        m_suppressed_columns.clear();

        for (auto it = m_retiring_columns.begin(); it != m_retiring_columns.end();) {
            if (!m_world.column_loaded(it->first) || tile_ready(it->second)) it = m_retiring_columns.erase(it);
            else ++it;
        }

        for (auto it = m_retiring_tiles.begin(); it != m_retiring_tiles.end();) {
            const TileTransition& tr = it->second;
            bool ready = true;
            for (const LodTileKey& k : tr.tiles) if (!tile_ready(k)) { ready = false; break; }
            if (ready) for (const ColumnPos& c : tr.columns) if (!column_ready(c)) { ready = false; break; }

            if (ready) {
                auto t = m_tiles.find(it->first);

                if (t != m_tiles.end() && !t->second.selected) {
                    uncount(t->second);

                    if (t->second.pending) {
                        m_pending_bytes -= t->second.pending->quads.memory_bytes();
                        --m_pending_uploads;
                    }

                    m_tiles.erase(t);
                }

                it = m_retiring_tiles.erase(it);
                continue;
            }

            for (const LodTileKey& k : tr.tiles) m_suppressed_tiles.insert(k);
            for (const ColumnPos& c : tr.columns) m_suppressed_columns.insert(c);
            ++it;
        }
    }

    World&                  m_world;
    const WorldGenerator&   m_generator;
    JobSystem&              m_jobs;
    ChunkStreamer&          m_streamer;
    ChunkMeshOptions        m_mesh_options;
    StreamingSettings       m_streaming;
    LodSettings             m_lod;
    LodLayout               m_layout;
    LodSelection            m_selection;
    vector3d                m_camera{};

    ChunkMap                m_chunks;
    TileMap                 m_tiles;
    std::unordered_map<ColumnPos, TerrainColumn, ColumnPosHash> m_columns;
    std::unordered_set<ChunkPos, ChunkPosHash>      m_wanted;
    std::unordered_set<LodTileKey, LodTileKeyHash>  m_tile_queue;
    std::vector<ChunkPos>   m_candidates;
    std::vector<LodTileKey> m_tile_candidates;
    std::deque<ChunkPos>    m_upload_order;
    std::deque<LodTileKey>  m_tile_upload_order;

    std::unordered_map<LodTileKey, TileTransition, LodTileKeyHash> m_retiring_tiles;
    std::unordered_map<ColumnPos, LodTileKey, ColumnPosHash>        m_retiring_columns;
    std::unordered_set<ColumnPos, ColumnPosHash>                    m_suppressed_columns;
    std::unordered_set<LodTileKey, LodTileKeyHash>                  m_suppressed_tiles;

    TerrainStats            m_totals;
    int                     m_mesh_jobs = 0;
    int                     m_tile_jobs = 0;
    std::size_t             m_pending_uploads = 0;
    std::size_t             m_pending_bytes = 0;
    std::uint64_t           m_gpu_bytes = 0;

    CaveCulling             m_cave;
    ChunkPos                m_cave_chunk{ 0, 0, std::numeric_limits<int>::min() };
    bool                    m_cave_dirty = true;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_TERRAIN_MESHES_HPP