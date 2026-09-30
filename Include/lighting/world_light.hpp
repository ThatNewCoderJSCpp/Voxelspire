#ifndef VOXELSPIRE_LIGHTING_WORLD_LIGHT_HPP
#define VOXELSPIRE_LIGHTING_WORLD_LIGHT_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../world/world.hpp"
#include "engine.hpp"

namespace voxelspire {

template <typename Format>
class WorldLight final : public LightEngine {
public:
    using Cell = typename Format::Cell;

    explicit WorldLight(const World& world)
        : m_world(world),
          m_min_z(world.settings().min_z), m_max_z(world.settings().max_z),
          m_min_cz(world.min_chunk_z()), m_levels(world.max_chunk_z() - world.min_chunk_z() + 1) {}

    LightFormat format() const noexcept override { return Format::FORMAT; }

    void column_loaded(const ColumnPos& c) override {
        if (m_pending_set.insert(c).second) m_pending.push_back(c);
    }

    void column_unloaded(const ColumnPos& c) override {
        forget_cache();
        auto it = m_columns.find(c);

        if (it != m_columns.end()) {
            m_sections -= it->second.section_count;
            m_emitter_count -= it->second.emitters.size();
            m_columns.erase(it);
        }

        if (m_pending_set.erase(c)) m_pending.erase(std::remove(m_pending.begin(), m_pending.end(), c), m_pending.end());
    }

    void block_changed(const BlockPos& p, BlockId before, BlockId after) override {
        Column* col = column(chunk_coord(p.x), chunk_coord(p.y));
        if (!col) return;
        const int level = level_of(p.z);
        if (level >= 0 && level < m_levels) col->chunks[static_cast<std::size_t>(level)] = m_world.chunk_at({ chunk_coord(p.x), chunk_coord(p.y), chunk_coord(p.z) });
        m_edits.push_back({ p, before, after });
    }

    void update(double budget_ms, ChunkChangeSink changed) override {
        const auto start = std::chrono::steady_clock::now();
        m_sink = changed;
        m_changed_cells = 0;
        m_last_marked = { 0, 0, std::numeric_limits<int>::min() };

        if (!m_edits.empty()) {
            m_edits_running.swap(m_edits);
            m_touched.clear();
            m_tracking = true;
            for (const Edit& e : m_edits_running) apply_edit(e);
            propagate();
            m_tracking = false;
            m_edits_running.clear();
            release_plain_sections();
        }

        bool first = true;

        while (!m_pending.empty()) {
            if (!first && elapsed_ms(start) >= budget_ms) break;
            const ColumnPos c = m_pending.front();
            m_pending.pop_front();
            m_pending_set.erase(c);
            if (m_world.column_loaded(c)) light_column(c);
            first = false;
        }

        m_sink = ChunkChangeSink();
        m_update_ms = elapsed_ms(start);
    }

    bool settled(const ColumnPos& c) const noexcept override {
        if (!lit(c)) return false;
        static constexpr int DX[4] = { -1, 1, 0, 0 }, DY[4] = { 0, 0, -1, 1 };

        for (int i = 0; i < 4; ++i) {
            const ColumnPos n{ c.x + DX[i], c.y + DY[i] };
            if (m_world.column_loaded(n) && !lit(n)) return false;
        }

        return true;
    }

    bool idle() const noexcept override { return m_pending.empty() && m_edits.empty(); }

    LightLevel level_at(const BlockPos& p) const noexcept override {
        if (p.z >= m_max_z) return LightLevel::open_sky();
        if (p.z < m_min_z) return LightLevel::dark();
        const Column* col = find_column(chunk_coord(p.x), chunk_coord(p.y));
        if (!col) return LightLevel::open_sky();
        return Format::level(read(*col, p.x, p.y, p.z));
    }

    void sample_box(const ChunkPos& chunk, std::uint16_t* out) const override {
        constexpr int S = Chunk::SIZE;
        const int ox = chunk.x * S, oy = chunk.y * S, oz = chunk.z * S;

        for (int y = -LightBox::MARGIN; y < S + LightBox::MARGIN; ++y)
            for (int x = -LightBox::MARGIN; x < S + LightBox::MARGIN; ++x) {
                const int wx = ox + x, wy = oy + y;
                const Column* col = find_column(chunk_coord(wx), chunk_coord(wy));

                for (int z = -LightBox::MARGIN; z < S + LightBox::MARGIN; ++z) {
                    const int wz = oz + z;
                    std::uint16_t v = PackedLight::OPEN_SKY;
                    if (wz < m_min_z) v = 0;
                    else if (col && wz < m_max_z) v = Format::packed(read(*col, wx, wy, wz));
                    out[LightBox::index(x, y, z)] = v;
                }
            }
    }

    void collect_emitters(const vector3d& center, double radius, std::vector<EmitterBlock>& out) const override {
        const BlockTraits* traits = m_world.blocks().traits_table();
        const int r = static_cast<int>(std::ceil(radius / Chunk::SIZE));
        const ColumnPos mid = World::column_of(center);
        const double r2 = radius * radius;

        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                const Column* col = find_column(mid.x + dx, mid.y + dy);
                if (!col) continue;

                for (const BlockPos& p : col->emitters) {
                    const vector3d d = p.center() - center;
                    if (d.x * d.x + d.y * d.y + d.z * d.z > r2) continue;
                    out.push_back({ p, traits[m_world.block_id_at(p)].emission });
                }
            }
    }

    LightStats stats() const override {
        LightStats s;
        s.format          = Format::FORMAT;
        s.columns         = m_columns.size();
        s.sections        = m_sections;
        s.bytes           = m_sections * sizeof(Section) + m_columns.size() * column_overhead();
        s.pending_columns = m_pending.size();
        s.emitters        = m_emitter_count;
        s.cells_changed   = m_changed_cells;
        s.update_ms       = m_update_ms;
        return s;
    }

private:
    static constexpr int S   = Chunk::SIZE;
    static constexpr int MAX = LightLimits::MAX;
    static constexpr int SKY = Format::SKY;
    static constexpr int DIRECTIONS = 6;
    static constexpr int DOWN = 4;
    static constexpr int NX[DIRECTIONS] = { -1, 1, 0, 0, 0, 0 };
    static constexpr int NY[DIRECTIONS] = { 0, 0, -1, 1, 0, 0 };
    static constexpr int NZ[DIRECTIONS] = { 0, 0, 0, 0, -1, 1 };

    using Section = std::array<Cell, Chunk::VOLUME>;
    using Height  = std::int16_t;

    static_assert(EngineLimits::WORLD_MIN_Z - 1 >= std::numeric_limits<Height>::min() && EngineLimits::WORLD_MAX_Z + 1 <= std::numeric_limits<Height>::max(),
                  "sky floors must fit the height type");

    struct Column {
        std::array<Height, ChunkColumn::AREA>       sky_floor{};
        std::vector<std::unique_ptr<Section>>       sections;
        std::vector<const Chunk*>                   chunks;
        std::vector<BlockPos>                       emitters;
        std::size_t                                 section_count = 0;
    };

    struct Node { int x, y, z; };
    struct Removal { int x, y, z, value; };
    struct Edit { BlockPos pos; BlockId before, after; };

    static int chunk_coord(int v) noexcept { return floor_div(v, S); }
    static int local(int v) noexcept { return floor_mod(v, S); }
    int level_of(int z) const noexcept { return chunk_coord(z) - m_min_cz; }

    std::size_t column_overhead() const noexcept {
        return sizeof(Column) + static_cast<std::size_t>(m_levels) * (sizeof(std::unique_ptr<Section>) + sizeof(const Chunk*));
    }

    static double elapsed_ms(std::chrono::steady_clock::time_point start) noexcept {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    }

    bool lit(const ColumnPos& c) const noexcept { return m_columns.count(c) != 0; }

    void forget_cache() const noexcept {
        m_cache_col = nullptr;
        m_cache_pos = { std::numeric_limits<int>::min(), std::numeric_limits<int>::min() };
    }

    const Column* find_column(int cx, int cy) const noexcept {
        if (m_cache_col && m_cache_pos.x == cx && m_cache_pos.y == cy) return m_cache_col;
        auto it = m_columns.find({ cx, cy });
        if (it == m_columns.end()) return nullptr;
        m_cache_pos = { cx, cy };
        m_cache_col = const_cast<Column*>(&it->second);
        return m_cache_col;
    }

    Column* column(int cx, int cy) noexcept { return const_cast<Column*>(find_column(cx, cy)); }

    static int cell_of(int x, int y) noexcept { return ChunkColumn::cell(local(x), local(y)); }

    Cell implicit(const Column& col, int x, int y, int z) const noexcept {
        return z >= col.sky_floor[static_cast<std::size_t>(cell_of(x, y))] ? Format::sky_only(MAX) : Cell(0);
    }

    Cell read(const Column& col, int x, int y, int z) const noexcept {
        const auto& sec = col.sections[static_cast<std::size_t>(level_of(z))];
        if (sec) return (*sec)[Chunk::index(local(x), local(y), local(z))];
        return implicit(col, x, y, z);
    }

    Section& materialize(Column& col, int level) {
        auto& sec = col.sections[static_cast<std::size_t>(level)];
        if (sec) return *sec;
        sec = std::make_unique<Section>();
        const int base_z = (level + m_min_cz) * S;

        for (int ly = 0; ly < S; ++ly)
            for (int lx = 0; lx < S; ++lx) {
                const int floor = col.sky_floor[static_cast<std::size_t>(ChunkColumn::cell(lx, ly))];
                for (int lz = 0; lz < S; ++lz)
                    (*sec)[Chunk::index(lx, ly, lz)] = base_z + lz >= floor ? Format::sky_only(MAX) : Cell(0);
            }

        ++col.section_count;
        ++m_sections;
        if (m_tracking) track(col, level);
        return *sec;
    }

    void write(Column& col, int x, int y, int z, Cell v) {
        const int level = level_of(z);
        auto& sec = col.sections[static_cast<std::size_t>(level)];

        if (!sec) {
            if (v == implicit(col, x, y, z)) return;
            materialize(col, level);
        }

        Cell& cell = (*sec)[Chunk::index(local(x), local(y), local(z))];
        if (cell == v) return;
        cell = v;
        if (m_tracking) track(col, level);
        note_changed(x, y, z);
    }

    void track(Column& col, int level) {
        if (!m_touched.empty() && m_touched.back().first == &col && m_touched.back().second == level) return;
        m_touched.emplace_back(&col, level);
    }

    bool matches_implicit(const Column& col, int level) const noexcept {
        const Section& sec = *col.sections[static_cast<std::size_t>(level)];
        const int base_z = (level + m_min_cz) * S;

        for (int ly = 0; ly < S; ++ly)
            for (int lx = 0; lx < S; ++lx) {
                const int floor = col.sky_floor[static_cast<std::size_t>(ChunkColumn::cell(lx, ly))];
                for (int lz = 0; lz < S; ++lz)
                    if (sec[Chunk::index(lx, ly, lz)] != (base_z + lz >= floor ? Format::sky_only(MAX) : Cell(0))) return false;
            }

        return true;
    }

    void release_plain_sections() {
        std::sort(m_touched.begin(), m_touched.end());
        m_touched.erase(std::unique(m_touched.begin(), m_touched.end()), m_touched.end());

        for (const auto& t : m_touched) {
            Column& col = *t.first;
            auto& sec = col.sections[static_cast<std::size_t>(t.second)];
            if (!sec || !matches_implicit(col, t.second)) continue;
            sec.reset();
            --col.section_count;
            --m_sections;
        }

        m_touched.clear();
    }

    void note_changed(int x, int y, int z) {
        ++m_changed_cells;
        if (!m_sink) return;
        const int cx = chunk_coord(x), cy = chunk_coord(y), cz = chunk_coord(z);
        const int lx = local(x), ly = local(y), lz = local(z);
        const int x0 = lx == 0 ? -1 : 0, x1 = lx == S - 1 ? 1 : 0;
        const int y0 = ly == 0 ? -1 : 0, y1 = ly == S - 1 ? 1 : 0;
        const int z0 = lz == 0 ? -1 : 0, z1 = lz == S - 1 ? 1 : 0;

        if (x0 == 0 && x1 == 0 && y0 == 0 && y1 == 0 && z0 == 0 && z1 == 0) {
            const ChunkPos p{ cx, cy, cz };
            if (p == m_last_marked) return;
            m_last_marked = p;
            m_sink(p);
            return;
        }

        for (int dz = z0; dz <= z1; ++dz)
            for (int dy = y0; dy <= y1; ++dy)
                for (int dx = x0; dx <= x1; ++dx) m_sink({ cx + dx, cy + dy, cz + dz });
    }

    BlockId block_at(const Column& col, int x, int y, int z) const noexcept {
        const Chunk* c = col.chunks[static_cast<std::size_t>(level_of(z))];
        return c ? c->get(local(x), local(y), local(z)) : AIR_ID;
    }

    const BlockTraits& traits_at(const Column& col, int x, int y, int z) const noexcept {
        return m_traits[block_at(col, x, y, z)];
    }

    bool in_height(int z) const noexcept { return z >= m_min_z && z < m_max_z; }

    void refresh_chunks(Column& col, const ColumnPos& c) {
        col.chunks.assign(static_cast<std::size_t>(m_levels), nullptr);
        const ChunkColumn* wc = m_world.column(c);
        if (!wc) return;

        for (int z : wc->chunk_zs) {
            const int level = z - m_min_cz;
            if (level >= 0 && level < m_levels) col.chunks[static_cast<std::size_t>(level)] = m_world.chunk_at({ c.x, c.y, z });
        }
    }

    int top_blocker(const Column& col, int x, int y, int below) const noexcept {
        for (int z = vmin(below, m_max_z) - 1; z >= m_min_z; --z) {
            const Chunk* c = col.chunks[static_cast<std::size_t>(level_of(z))];

            if (!c) {
                z = (chunk_coord(z)) * S;
                continue;
            }

            if (m_traits[c->get(local(x), local(y), local(z))].light_opacity > 0) return z;
        }

        return m_min_z - 1;
    }

    void compute_floors(Column& col, const ColumnPos& c) {
        for (int ly = 0; ly < S; ++ly)
            for (int lx = 0; lx < S; ++lx)
                col.sky_floor[static_cast<std::size_t>(ChunkColumn::cell(lx, ly))] = static_cast<Height>(top_blocker(col, c.x * S + lx, c.y * S + ly, m_max_z) + 1);
    }

    void scan_emitters(Column& col) {
        col.emitters.clear();

        for (int level = 0; level < m_levels; ++level) {
            const Chunk* chunk = col.chunks[static_cast<std::size_t>(level)];
            if (!chunk) continue;
            const BlockId* ids = chunk->data();
            const BlockPos o = chunk->origin();

            for (int ly = 0; ly < S; ++ly)
                for (int lz = 0; lz < S; ++lz)
                    for (int lx = 0; lx < S; ++lx)
                        if (m_traits[ids[Chunk::index(lx, ly, lz)]].emission.emits()) col.emitters.push_back({ o.x + lx, o.y + ly, o.z + lz });
        }
    }

    int floor_at(int x, int y) const noexcept {
        const Column* col = find_column(chunk_coord(x), chunk_coord(y));
        return col ? col->sky_floor[static_cast<std::size_t>(cell_of(x, y))] : std::numeric_limits<int>::max();
    }

    void push(int x, int y, int z) { m_queue.push_back({ x, y, z }); }

    void light_column(const ColumnPos& c) {
        m_traits = m_world.blocks().traits_table();
        Column& col = m_columns[c];
        col.sections.clear();
        col.sections.resize(static_cast<std::size_t>(m_levels));
        refresh_chunks(col, c);
        compute_floors(col, c);
        scan_emitters(col);
        m_emitter_count += col.emitters.size();
        const int bx = c.x * S, by = c.y * S;

        for (int ly = 0; ly < S; ++ly)
            for (int lx = 0; lx < S; ++lx) {
                const int x = bx + lx, y = by + ly;
                const int floor = col.sky_floor[static_cast<std::size_t>(ChunkColumn::cell(lx, ly))];
                int v = MAX;

                for (int z = floor - 1; z >= m_min_z; --z) {
                    const int op = traits_at(col, x, y, z).light_opacity;
                    if (op >= MAX) break;
                    v = v == MAX ? MAX - op : v - 1 - op;
                    if (v <= 0) break;
                    write(col, x, y, z, Format::set(read(col, x, y, z), SKY, v));
                    push(x, y, z);
                }

                for (int d = 0; d < 4; ++d) {
                    const int nx = x + NX[d], ny = y + NY[d];
                    const bool inside = chunk_coord(nx) == c.x && chunk_coord(ny) == c.y;
                    const int nfloor = floor_at(nx, ny);
                    if (nfloor == std::numeric_limits<int>::max()) continue;
                    for (int z = floor; z < vmin(nfloor, m_max_z); ++z) push(x, y, z);
                    if (!inside) for (int z = nfloor; z < vmin(floor, m_max_z); ++z) push(nx, ny, z);
                }
            }

        for (const BlockPos& e : col.emitters) {
            const LightEmission& em = traits_at(col, e.x, e.y, e.z).emission;
            Cell v = read(col, e.x, e.y, e.z);
            for (int ch = 0; ch < Format::CHANNELS; ++ch) if (ch != SKY) v = Format::set(v, ch, vmax(Format::get(v, ch), Format::emission(em, ch)));
            write(col, e.x, e.y, e.z, v);
            push(e.x, e.y, e.z);
        }

        seed_from_neighbors(c);
        propagate();
    }

    void seed_from_neighbors(const ColumnPos& c) {
        static constexpr int DX[4] = { -1, 1, 0, 0 }, DY[4] = { 0, 0, -1, 1 };

        for (int d = 0; d < 4; ++d) {
            const ColumnPos n{ c.x + DX[d], c.y + DY[d] };
            const Column* col = find_column(n.x, n.y);
            if (!col) continue;
            const int fixed_x = DX[d] < 0 ? n.x * S + S - 1 : (DX[d] > 0 ? n.x * S : 0);
            const int fixed_y = DY[d] < 0 ? n.y * S + S - 1 : (DY[d] > 0 ? n.y * S : 0);

            for (int level = 0; level < m_levels; ++level) {
                const auto& sec = col->sections[static_cast<std::size_t>(level)];
                if (!sec) continue;
                const int z0 = (level + m_min_cz) * S;

                for (int i = 0; i < S; ++i) {
                    const int x = DX[d] != 0 ? fixed_x : n.x * S + i;
                    const int y = DY[d] != 0 ? fixed_y : n.y * S + i;
                    for (int lz = 0; lz < S; ++lz) if ((*sec)[Chunk::index(local(x), local(y), lz)] != 0) push(x, y, z0 + lz);
                }
            }
        }
    }

    void propagate() {
        std::size_t head = 0;

        while (head < m_queue.size()) {
            const Node n = m_queue[head++];
            const Column* src_col = find_column(chunk_coord(n.x), chunk_coord(n.y));
            if (!src_col) continue;
            const Cell src = read(*src_col, n.x, n.y, n.z);
            if (src == 0) continue;

            for (int d = 0; d < DIRECTIONS; ++d) {
                const int x = n.x + NX[d], y = n.y + NY[d], z = n.z + NZ[d];
                if (!in_height(z)) continue;
                Column* col = column(chunk_coord(x), chunk_coord(y));
                if (!col) continue;
                const int op = traits_at(*col, x, y, z).light_opacity;
                if (op >= MAX) continue;
                const Cell cur = read(*col, x, y, z);
                Cell next = cur;

                for (int ch = 0; ch < Format::CHANNELS; ++ch) {
                    const int s = Format::get(src, ch);
                    if (s <= 0) continue;
                    const int cand = (ch == SKY && d == DOWN && s == MAX) ? MAX - op : s - 1 - op;
                    if (cand > Format::get(next, ch)) next = Format::set(next, ch, cand);
                }

                if (next == cur) continue;
                write(*col, x, y, z, next);
                push(x, y, z);
            }
        }

        m_queue.clear();
    }

    int emission_at(const Column& col, int x, int y, int z, int ch) const noexcept {
        return ch == SKY ? 0 : Format::emission(traits_at(col, x, y, z).emission, ch);
    }

    void remove_channel(int ch, int x, int y, int z, int value) {
        m_removals.clear();
        m_removals.push_back({ x, y, z, value });
        std::size_t head = 0;

        while (head < m_removals.size()) {
            const Removal r = m_removals[head++];

            for (int d = 0; d < DIRECTIONS; ++d) {
                const int nx = r.x + NX[d], ny = r.y + NY[d], nz = r.z + NZ[d];
                if (!in_height(nz)) continue;
                Column* col = column(chunk_coord(nx), chunk_coord(ny));
                if (!col) continue;
                const Cell cur = read(*col, nx, ny, nz);
                const int v = Format::get(cur, ch);
                if (v == 0) continue;
                const bool direct = ch == SKY && d == DOWN && r.value == MAX && v == MAX;

                if (v < r.value || direct) {
                    const int keep = emission_at(*col, nx, ny, nz, ch);
                    write(*col, nx, ny, nz, Format::set(cur, ch, keep));
                    m_removals.push_back({ nx, ny, nz, v });
                    if (keep > 0) push(nx, ny, nz);
                } else {
                    push(nx, ny, nz);
                }
            }
        }
    }

    void apply_edit(const Edit& e) {
        m_traits = m_world.blocks().traits_table();
        const BlockPos& p = e.pos;
        if (!in_height(p.z)) return;
        Column* col = column(chunk_coord(p.x), chunk_coord(p.y));
        if (!col) return;
        const BlockTraits& was = m_traits[e.before];
        const BlockTraits& now = m_traits[e.after];
        const int cell = cell_of(p.x, p.y);
        update_emitters(*col, p, was.emission.emits(), now.emission.emits());

        if (now.light_opacity > 0 && p.z >= col->sky_floor[static_cast<std::size_t>(cell)]) {
            const int old_floor = col->sky_floor[static_cast<std::size_t>(cell)];
            for (int level = level_of(old_floor); level <= level_of(p.z); ++level) materialize(*col, level);
            col->sky_floor[static_cast<std::size_t>(cell)] = static_cast<Height>(p.z + 1);
        } else if (now.light_opacity == 0 && was.light_opacity > 0 && p.z + 1 == col->sky_floor[static_cast<std::size_t>(cell)]) {
            const int new_floor = top_blocker(*col, p.x, p.y, p.z) + 1;
            col->sky_floor[static_cast<std::size_t>(cell)] = static_cast<Height>(new_floor);

            for (int z = p.z; z >= new_floor; --z) {
                const Cell cur = read(*col, p.x, p.y, z);
                write(*col, p.x, p.y, z, Format::set(cur, SKY, MAX));
                push(p.x, p.y, z);
            }
        }

        const bool denser = now.light_opacity > was.light_opacity;

        for (int ch = 0; ch < Format::CHANNELS; ++ch) {
            const int v = Format::get(read(*col, p.x, p.y, p.z), ch);
            const int glow = emission_at(*col, p.x, p.y, p.z, ch);
            const int old_glow = ch == SKY ? 0 : Format::emission(was.emission, ch);

            if (v > 0 && (denser || old_glow > glow)) {
                write(*col, p.x, p.y, p.z, Format::set(read(*col, p.x, p.y, p.z), ch, glow));
                remove_channel(ch, p.x, p.y, p.z, v);
                if (glow > 0) push(p.x, p.y, p.z);
            } else if (glow > v) {
                write(*col, p.x, p.y, p.z, Format::set(read(*col, p.x, p.y, p.z), ch, glow));
                push(p.x, p.y, p.z);
            }
        }

        if (now.light_opacity < was.light_opacity) {
            for (int d = 0; d < DIRECTIONS; ++d) {
                const int nx = p.x + NX[d], ny = p.y + NY[d], nz = p.z + NZ[d];
                if (in_height(nz) && find_column(chunk_coord(nx), chunk_coord(ny))) push(nx, ny, nz);
            }
        }
    }

    void update_emitters(Column& col, const BlockPos& p, bool was, bool now) {
        if (was == now) return;

        if (now) {
            col.emitters.push_back(p);
            ++m_emitter_count;
            return;
        }

        auto it = std::find(col.emitters.begin(), col.emitters.end(), p);
        if (it == col.emitters.end()) return;
        col.emitters.erase(it);
        --m_emitter_count;
    }

    const World&                                           m_world;
    const BlockTraits*                                     m_traits = nullptr;
    int                                                    m_min_z, m_max_z, m_min_cz, m_levels;
    std::unordered_map<ColumnPos, Column, ColumnPosHash>   m_columns;
    std::deque<ColumnPos>                                  m_pending;
    std::unordered_set<ColumnPos, ColumnPosHash>           m_pending_set;
    std::vector<Edit>                                      m_edits;
    std::vector<Edit>                                      m_edits_running;
    std::vector<std::pair<Column*, int>>                   m_touched;
    bool                                                   m_tracking = false;
    std::vector<Node>                                      m_queue;
    std::vector<Removal>                                   m_removals;
    ChunkChangeSink                                        m_sink;
    ChunkPos                                               m_last_marked{};
    std::size_t                                            m_sections = 0;
    std::size_t                                            m_emitter_count = 0;
    std::size_t                                            m_changed_cells = 0;
    double                                                 m_update_ms = 0.0;
    mutable ColumnPos                                      m_cache_pos{ std::numeric_limits<int>::min(), std::numeric_limits<int>::min() };
    mutable Column*                                        m_cache_col = nullptr;
};

inline std::unique_ptr<LightEngine> make_light_engine(const World& world, LightFormat format) {
    switch (format) {
        case LightFormat::Plain:   return std::make_unique<WorldLight<PlainLight>>(world);
        case LightFormat::Colored: return std::make_unique<WorldLight<ColoredLight>>(world);
        case LightFormat::None:    break;
    }

    return nullptr;
}

} // namespace voxelspire

#endif // VOXELSPIRE_LIGHTING_WORLD_LIGHT_HPP