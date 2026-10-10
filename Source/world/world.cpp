#include "world/world.hpp"

namespace voxelspire {

void World::set_physics(const WorldSettings& physics) {
    m_settings.gravity          = physics.gravity;
    m_settings.air_resistance   = physics.air_resistance;
    m_settings.fluid_resistance = physics.fluid_resistance;
    m_settings.fluid_buoyancy   = physics.fluid_buoyancy;
    m_settings.fluid_sink_speed = physics.fluid_sink_speed;
    m_settings.current_speed    = physics.current_speed;
    m_settings.current_push     = physics.current_push;
    m_settings.wade_slowdown    = physics.wade_slowdown;
    m_settings.fall_break_depth = physics.fall_break_depth;
    m_settings.fluid_updates    = physics.fluid_updates;
    const bool rules_changed = physics.fluid_rules != m_settings.fluid_rules;
    m_settings.fluid_rules      = physics.fluid_rules;
    m_settings = m_settings.validated();
    if (rules_changed) wake_all_fluids();
}

ChunkPos World::chunk_pos_of(const BlockPos& p) noexcept {
    return { floor_div(p.x, Chunk::SIZE), floor_div(p.y, Chunk::SIZE), floor_div(p.z, Chunk::SIZE) };
}

bool World::in_horizontal_bounds(const BlockPos& p) const noexcept {
    const int limit = m_settings.horizontal_limit;
    return p.x >= -limit && p.x < limit && p.y >= -limit && p.y < limit;
}

bool World::column_in_bounds(const ColumnPos& c) const noexcept {
    return in_horizontal_bounds({ c.x * Chunk::SIZE, c.y * Chunk::SIZE, 0 })
        || in_horizontal_bounds({ c.x * Chunk::SIZE + Chunk::SIZE - 1, c.y * Chunk::SIZE + Chunk::SIZE - 1, 0 });
}

BlockId World::block_id_at(const BlockPos& p) const noexcept {
    const Chunk* c = chunk_at(chunk_pos_of(p));
    if (!c) return AIR_ID;
    return c->get(floor_mod(p.x, Chunk::SIZE), floor_mod(p.y, Chunk::SIZE), floor_mod(p.z, Chunk::SIZE));
}

std::uint8_t World::fluid_state(const BlockPos& p) const noexcept {
    const Chunk* c = chunk_at(chunk_pos_of(p));
    if (!c) return 0;
    return c->state(floor_mod(p.x, Chunk::SIZE), floor_mod(p.y, Chunk::SIZE), floor_mod(p.z, Chunk::SIZE));
}

double World::fluid_height(const BlockPos& p) const noexcept {
    const BlockId id = block_id_at(p);
    if (!m_registry->traits(id).fluid) return 0.0;
    if (block_id_at({ p.x, p.y, p.z + 1 }) == id) return 1.0;
    return fluid_level(p, fluid_state(p));
}

double World::fluid_level(const BlockPos& p, std::uint8_t state) const noexcept {
    const double level = m_settings.fluid_rules->level(state);
    if (m_occupancy.empty()) return level;
    auto it = m_occupancy.find(p);
    return it == m_occupancy.end() ? level : vmin(it->second.level(level), 1.0);
}

FluidOccupancy World::occupancy(const BlockPos& p) const noexcept {
    if (m_occupancy.empty()) return {};
    auto it = m_occupancy.find(p);
    return it == m_occupancy.end() ? FluidOccupancy{} : it->second;
}

void World::displace(const std::vector<FluidDisplacer>& bodies) {
    m_next_occupancy.clear();
    if (m_settings.fluid_rules->displaces()) for (const FluidDisplacer& body : bodies) occupy(body.box, body.share);

    for (const auto& kv : m_next_occupancy) {
        auto it = m_occupancy.find(kv.first);
        if (it == m_occupancy.end() || it->second != kv.second) occupancy_changed(kv.first);
    }

    for (const auto& kv : m_occupancy)
        if (m_next_occupancy.count(kv.first) == 0) occupancy_changed(kv.first);

    m_occupancy.swap(m_next_occupancy);
}

vector3d World::fluid_flow(const BlockPos& p) const noexcept {
    const BlockId id = block_id_at(p);
    if (!m_registry->traits(id).fluid) return {};
    const double h = fluid_height(p);
    vector3d flow{};

    for (const BlockPos& d : HORIZONTAL_STEPS) {
        const BlockPos q{ p.x + d.x, p.y + d.y, p.z };
        const BlockId qid = block_id_at(q);
        double hq = 0.0;
        if (qid == id) hq = fluid_height(q);
        else if (m_registry->traits(qid).solid || !column_loaded(column_of(q))) continue;
        flow.x += d.x * (h - hq);
        flow.y += d.y * (h - hq);
    }

    const double len = std::sqrt(flow.x * flow.x + flow.y * flow.y);
    if (len > 1.0) { flow.x /= len; flow.y /= len; }
    const BlockPos below{ p.x, p.y, p.z - 1 };
    const BlockId bid = block_id_at(below);

    const bool falls = m_settings.fluid_rules->falling(fluid_state(p))
                    || (bid == AIR_ID && column_loaded(column_of(below)))
                    || (bid == id && m_settings.fluid_rules->falling(fluid_state(below)));

    if (falls && m_settings.fluid_rules->flows()) flow.z = -1.0;
    return flow;
}

void World::attach_lighting(std::unique_ptr<LightEngine> engine) {
    m_light = std::move(engine);
    if (!m_light) return;
    for (const auto& kv : m_columns) m_light->column_loaded(kv.first);
}

void World::update_lighting(double budget_ms) {
    if (!m_light) return;
    m_light->update(budget_ms, [this](const ChunkPos& p) { mark_changed(p); });
}

bool World::set_block(const BlockPos& p, BlockId id, std::uint8_t state) {
    if (!in_build_range(p)) return false;
    const ColumnPos col = column_of(p);
    auto colit = m_columns.find(col);
    if (colit == m_columns.end()) return false;
    const ChunkPos cp = chunk_pos_of(p);
    Chunk* c = chunk_at(cp);

    if (!c) {
        if (id == AIR_ID) return false;
        c = &add_chunk(colit->second, std::make_unique<Chunk>(cp));
    }

    const int lx = floor_mod(p.x, Chunk::SIZE), ly = floor_mod(p.y, Chunk::SIZE), lz = floor_mod(p.z, Chunk::SIZE);
    const BlockId before = c->get(lx, ly, lz);
    if (!c->set(lx, ly, lz, id, state)) return false;
    if (m_light && before != id) m_light->block_changed(p, before, id);

    mark_around(p);
    ++m_revision;
    if (before != id) update_floor(colit->second, col, lx, ly, p.z, id);
    if (m_edited_set.insert(col).second) m_edited.push_back(col);
    wake_around(p);
    return true;
}

void World::wake_all_fluids() {
    if (!m_settings.fluid_rules->flows()) return;
    for (const auto& kv : m_columns) wake_column(kv.first);
}

void World::insert_column(const ColumnPos& col, std::vector<std::unique_ptr<Chunk>> chunks) {
    if (column_loaded(col)) return;
    ChunkColumn& column = m_columns[col];

    for (auto& chunk : chunks) {
        if (!chunk || chunk->empty()) continue;
        add_chunk(column, std::move(chunk));
    }

    rebuild_floor(column, col);
    ++m_revision;
    for (int z : column.chunk_zs) mark_changed({ col.x, col.y, z });
    m_column_events.emplace_back(col, ColumnEvent::Loaded);

    if (m_settings.fluid_rules->flows()) {
        wake_column(col);
        for (const BlockPos& d : HORIZONTAL_STEPS) wake_column_edge({ col.x + d.x, col.y + d.y }, d);
    }

    if (m_light) m_light->column_loaded(col);
}

std::vector<std::unique_ptr<Chunk>> World::remove_column(const ColumnPos& col) {
    std::vector<std::unique_ptr<Chunk>> out;
    auto it = m_columns.find(col);
    if (it == m_columns.end()) return out;

    for (int z : it->second.chunk_zs) {
        auto cit = m_chunks.find({ col.x, col.y, z });
        if (cit == m_chunks.end()) continue;
        out.push_back(std::move(cit->second));
        m_chunks.erase(cit);
    }

    m_columns.erase(it);
    ++m_revision;
    m_column_events.emplace_back(col, ColumnEvent::Unloaded);
    if (m_light) m_light->column_unloaded(col);
    return out;
}

void World::mark_around(const BlockPos& p) {
    for (int dz = -1; dz <= 1; ++dz)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) mark_changed(chunk_pos_of({ p.x + dx, p.y + dy, p.z + dz }));
}

void World::occupy(const AABB& box, double share) {
    const BlockPos lo = BlockPos::containing(box.min);
    const BlockPos hi = BlockPos::containing(box.max);
    const BlockTraits* traits = m_registry->traits_table();

    for (int z = lo.z; z <= hi.z; ++z)
        for (int y = lo.y; y <= hi.y; ++y)
            for (int x = lo.x; x <= hi.x; ++x) {
                const BlockPos p{ x, y, z };
                const BlockId id = block_id_at(p);
                if (id != AIR_ID && !traits[id].fluid) continue;
                const double wx = vmin(box.max.x, x + 1.0) - vmax(box.min.x, static_cast<double>(x));
                const double wy = vmin(box.max.y, y + 1.0) - vmax(box.min.y, static_cast<double>(y));
                const double bottom = box.min.z - z, top = box.max.z - z;
                const FluidOccupancy occ = FluidOccupancy::of(wx * wy * vclamp(share, 0.0, 1.0), bottom, top);
                if (!occ.empty()) m_next_occupancy[p].merge(occ);
            }
}

void World::wake_around(const BlockPos& p) {
    if (!m_settings.fluid_rules->flows()) return;
    for (const BlockPos& d : WAKE_STEPS) m_fluid_wakes.push_back({ p.x + d.x, p.y + d.y, p.z + d.z });
}

bool World::spills(const BlockPos& p) const noexcept {
    for (const BlockPos& d : SPILL_STEPS) {
        const BlockPos q{ p.x + d.x, p.y + d.y, p.z + d.z };
        if (block_id_at(q) == AIR_ID && in_build_range(q) && column_loaded(column_of(q))) return true;
    }

    return false;
}

void World::wake_chunk(const Chunk& c, int x0, int x1, int y0, int y1) {
    const BlockTraits* traits = m_registry->traits_table();
    const BlockPos o = c.origin();

    for (int ly = y0; ly < y1; ++ly)
        for (int lz = 0; lz < Chunk::SIZE; ++lz)
            for (int lx = x0; lx < x1; ++lx) {
                if (!traits[c.get(lx, ly, lz)].fluid) continue;
                const BlockPos p{ o.x + lx, o.y + ly, o.z + lz };
                if (spills(p)) m_fluid_wakes.push_back(p);
            }
}

void World::wake_column(const ColumnPos& col) {
    auto it = m_columns.find(col);
    if (it == m_columns.end()) return;

    for (int cz : it->second.chunk_zs)
        if (const Chunk* c = chunk_at({ col.x, col.y, cz })) wake_chunk(*c, 0, Chunk::SIZE, 0, Chunk::SIZE);
}

void World::wake_column_edge(const ColumnPos& col, const BlockPos& side) {
    auto it = m_columns.find(col);
    if (it == m_columns.end()) return;
    const int x0 = side.x < 0 ? Chunk::SIZE - 1 : 0, x1 = side.x > 0 ? 1 : Chunk::SIZE;
    const int y0 = side.y < 0 ? Chunk::SIZE - 1 : 0, y1 = side.y > 0 ? 1 : Chunk::SIZE;

    for (int cz : it->second.chunk_zs)
        if (const Chunk* c = chunk_at({ col.x, col.y, cz })) wake_chunk(*c, x0, x1, y0, y1);
}

Chunk& World::add_chunk(ChunkColumn& column, std::unique_ptr<Chunk> chunk) {
    Chunk& ref = *chunk;
    const ChunkPos cp = chunk->pos();
    column.chunk_zs.push_back(cp.z);
    m_chunks[cp] = std::move(chunk);
    return ref;
}

void World::mark_changed(const ChunkPos& cp) {
    if (!column_loaded(column_of(cp))) return;
    if (m_changed_set.insert(cp).second) m_changed.push_back(cp);
}

void World::rebuild_floor(ChunkColumn& column, const ColumnPos& col) {
    column.floor.fill(ChunkColumn::NO_FLOOR);
    std::vector<int> zs = column.chunk_zs;
    std::sort(zs.begin(), zs.end());
    int remaining = ChunkColumn::AREA;

    for (int cz : zs) {
        const Chunk* c = chunk_at({ col.x, col.y, cz });
        if (!c || c->empty()) continue;

        for (int ly = 0; ly < Chunk::SIZE && remaining > 0; ++ly)
            for (int lx = 0; lx < Chunk::SIZE; ++lx) {
                std::int32_t& f = column.floor[ChunkColumn::cell(lx, ly)];
                if (f != ChunkColumn::NO_FLOOR) continue;
                for (int lz = 0; lz < Chunk::SIZE; ++lz)
                    if (c->get(lx, ly, lz) != AIR_ID) { f = cz * Chunk::SIZE + lz; --remaining; break; }
            }

        if (remaining == 0) break;
    }
}

void World::update_floor(ChunkColumn& column, const ColumnPos& col, int lx, int ly, int z, BlockId id) {
    std::int32_t& f = column.floor[ChunkColumn::cell(lx, ly)];
    const std::int32_t old = f;

    if (id != AIR_ID) {
        if (z < f) f = z;
    } else if (z == f) {
        f = ChunkColumn::NO_FLOOR;
        const int x = col.x * Chunk::SIZE + lx, y = col.y * Chunk::SIZE + ly;
        for (int zz = z + 1; zz < m_settings.max_z; ++zz)
            if (block_id_at({ x, y, zz }) != AIR_ID) { f = zz; break; }
    }

    if (f == old) return;
    if (old != ChunkColumn::NO_FLOOR) mark_changed({ col.x, col.y, floor_div(old, Chunk::SIZE) });
    if (f != ChunkColumn::NO_FLOOR)   mark_changed({ col.x, col.y, floor_div(f, Chunk::SIZE) });
}

} // namespace voxelspire
