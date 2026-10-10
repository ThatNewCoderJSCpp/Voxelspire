#include "world/fluid_simulator.hpp"

namespace voxelspire {

void FluidSimulator::update(World& world, double dt, const vector3d& center, double radius) {
    const FluidRules& rules = world.fluid_rules();
    absorb(world.take_fluid_wakes());

    if (!rules.flows()) {
        clear();
        return;
    }

    m_recheck += dt;

    if (m_recheck >= DORMANT_RECHECK) {
        m_recheck = 0.0;
        wake_dormant(center, radius);
    }

    const double step = rules.interval();
    m_time += dt;
    int steps = 0;

    while (m_time >= step && steps < MAX_STEPS_PER_UPDATE) {
        m_time -= step;
        run_step(world, rules, center, radius);
        absorb(world.take_fluid_wakes());
        ++steps;
    }

    m_time = vmin(m_time, step);
    m_stats.pending = m_pending.size();
    m_stats.dormant = m_dormant.size();
}

void FluidSimulator::clear() {
    m_pending.clear();
    m_pending_set.clear();
    m_dormant.clear();
    m_speeds.clear();
    m_splashes.clear();
    m_time = 0.0;
    m_stats = {};
}

std::vector<FluidSplash> FluidSimulator::take_splashes() {
    std::vector<FluidSplash> out;
    out.swap(m_splashes);
    if (m_speeds.size() > MAX_SPEEDS) m_speeds.clear();
    return out;
}

bool FluidSimulator::Access::open(const BlockPos& p) const {
    return m_world.block_id_at(p) == AIR_ID && m_world.in_build_range(p) && m_world.column_loaded(World::column_of(p));
}

FluidCell FluidSimulator::Access::look(const BlockPos& p) const {
    const BlockId id = m_world.block_id_at(p);
    if (id == m_fluid) return { false, true, m_world.fluid_state(p) };
    return { id == AIR_ID && m_world.in_build_range(p) && m_world.column_loaded(World::column_of(p)), false, 0 };
}

void FluidSimulator::wake_dormant(const vector3d& center, double radius) {
    auto keep = std::partition(m_dormant.begin(), m_dormant.end(), [&](const BlockPos& p) { return !within(p, center, radius); });
    for (auto it = keep; it != m_dormant.end(); ++it) if (m_pending_set.insert(*it).second) m_pending.push_back(*it);
    m_dormant.erase(keep, m_dormant.end());
}

void FluidSimulator::run_step(World& world, const FluidRules& rules, const vector3d& center, double radius) {
    m_current.clear();
    m_current.swap(m_pending);
    m_pending_set.clear();
    std::sort(m_current.begin(), m_current.end(), before);
    const BlockTraits* traits = world.blocks().traits_table();
    const std::size_t budget = static_cast<std::size_t>(vmax(world.settings().fluid_updates, 1));
    Access access(world, m_speeds, m_splashes);
    std::size_t updated = 0;

    for (const BlockPos& p : m_current) {
        const BlockId id = world.block_id_at(p);
        if (!traits[id].fluid) continue;

        if (!within(p, center, radius)) {
            m_dormant.push_back(p);
            continue;
        }

        if (updated >= budget) {
            if (m_pending_set.insert(p).second) m_pending.push_back(p);
            continue;
        }

        access.bind(id);
        rules.update(access, p);
        ++updated;
    }

    m_stats.updated = updated;
}

} // namespace voxelspire
