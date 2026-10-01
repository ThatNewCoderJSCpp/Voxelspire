#ifndef VOXELSPIRE_WORLD_FLUID_SIMULATOR_HPP
#define VOXELSPIRE_WORLD_FLUID_SIMULATOR_HPP

#include <algorithm>
#include <tuple>
#include <unordered_set>
#include <vector>
#include "world.hpp"

namespace voxelspire {

struct FluidStats {
    std::size_t pending  = 0;
    std::size_t dormant  = 0;
    std::size_t updated  = 0;
};

class FluidSimulator {
public:
    static constexpr int    MAX_STEPS_PER_UPDATE = 4;
    static constexpr double DORMANT_RECHECK      = 1.0;

    void update(World& world, double dt, const vector3d& center, double radius) {
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

    void clear() {
        m_pending.clear();
        m_pending_set.clear();
        m_dormant.clear();
        m_time = 0.0;
        m_stats = {};
    }

    const FluidStats& stats() const noexcept { return m_stats; }

private:
    class Access final : public FluidAccess {
    public:
        explicit Access(World& world) noexcept : m_world(world) {}

        void bind(BlockId fluid) noexcept { m_fluid = fluid; }

        bool same(const BlockPos& p) const override { return m_world.block_id_at(p) == m_fluid; }

        bool open(const BlockPos& p) const override {
            return m_world.block_id_at(p) == AIR_ID && m_world.in_build_range(p) && m_world.column_loaded(World::column_of(p));
        }

        std::uint8_t state(const BlockPos& p) const override { return m_world.fluid_state(p); }
        void put(const BlockPos& p, std::uint8_t state) override { m_world.set_block(p, m_fluid, state); }
        void clear(const BlockPos& p) override { m_world.set_block(p, AIR_ID); }

    private:
        World&  m_world;
        BlockId m_fluid = AIR_ID;
    };

    static bool before(const BlockPos& a, const BlockPos& b) noexcept {
        return std::tie(a.z, a.y, a.x) < std::tie(b.z, b.y, b.x);
    }

    static bool within(const BlockPos& p, const vector3d& center, double radius) noexcept {
        const double dx = p.x + 0.5 - center.x, dy = p.y + 0.5 - center.y;
        return dx * dx + dy * dy <= radius * radius;
    }

    void absorb(const std::vector<BlockPos>& wakes) {
        for (const BlockPos& p : wakes)
            if (m_pending_set.insert(p).second) m_pending.push_back(p);
    }

    void wake_dormant(const vector3d& center, double radius) {
        auto keep = std::partition(m_dormant.begin(), m_dormant.end(), [&](const BlockPos& p) { return !within(p, center, radius); });
        for (auto it = keep; it != m_dormant.end(); ++it) if (m_pending_set.insert(*it).second) m_pending.push_back(*it);
        m_dormant.erase(keep, m_dormant.end());
    }

    void run_step(World& world, const FluidRules& rules, const vector3d& center, double radius) {
        m_current.clear();
        m_current.swap(m_pending);
        m_pending_set.clear();
        std::sort(m_current.begin(), m_current.end(), before);
        const BlockTraits* traits = world.blocks().traits_table();
        const std::size_t budget = static_cast<std::size_t>(vmax(world.settings().fluid_updates, 1));
        Access access(world);
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

    std::vector<BlockPos>                          m_pending;
    std::unordered_set<BlockPos, BlockPosHash>     m_pending_set;
    std::vector<BlockPos>                          m_current;
    std::vector<BlockPos>                          m_dormant;
    double                                         m_time    = 0.0;
    double                                         m_recheck = 0.0;
    FluidStats                                     m_stats;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_FLUID_SIMULATOR_HPP