#ifndef VOXELSPIRE_WORLD_FLUID_SIMULATOR_HPP
#define VOXELSPIRE_WORLD_FLUID_SIMULATOR_HPP

#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "world.hpp"

namespace voxelspire {

struct FluidSplash {
    BlockPos pos;
    double   strength = 0.0;
};

struct FluidStats {
    std::size_t pending  = 0;
    std::size_t dormant  = 0;
    std::size_t updated  = 0;
};

class FluidSimulator {
public:
    static constexpr int         MAX_STEPS_PER_UPDATE = 4;
    static constexpr double      DORMANT_RECHECK      = 1.0;
    static constexpr std::size_t MAX_SPEEDS           = 65536;

    void update(World& world, double dt, const vector3d& center, double radius);

    void clear();

    const FluidStats& stats() const noexcept { return m_stats; }

    std::vector<FluidSplash> take_splashes();

private:
    class Access final : public FluidAccess {
    public:
        Access(World& world, std::unordered_map<BlockPos, float, BlockPosHash>& speeds, std::vector<FluidSplash>& splashes) noexcept
            : m_world(world), m_speeds(speeds), m_splashes(splashes) {}

        void bind(BlockId fluid) noexcept { m_fluid = fluid; }

        bool same(const BlockPos& p) const override { return m_world.block_id_at(p) == m_fluid; }

        bool open(const BlockPos& p) const override;

        std::uint8_t state(const BlockPos& p) const override { return m_world.fluid_state(p); }
        void put(const BlockPos& p, std::uint8_t state) override { m_world.set_block(p, m_fluid, state); }
        void clear(const BlockPos& p) override { m_world.set_block(p, AIR_ID); m_speeds.erase(p); }

        double speed(const BlockPos& p) const override {
            auto it = m_speeds.find(p);
            return it == m_speeds.end() ? 0.0 : it->second;
        }

        void set_speed(const BlockPos& p, double v) override {
            if (v <= 0.0) m_speeds.erase(p);
            else m_speeds[p] = static_cast<float>(v);
        }

        void splash(const BlockPos& p, double strength) override { m_splashes.push_back({ p, strength }); }
        FluidOccupancy occupancy(const BlockPos& p) const override { return m_world.occupancy(p); }

        FluidCell look(const BlockPos& p) const override;

    private:
        World&                                             m_world;
        std::unordered_map<BlockPos, float, BlockPosHash>& m_speeds;
        std::vector<FluidSplash>&                          m_splashes;
        BlockId                                            m_fluid = AIR_ID;
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

    void wake_dormant(const vector3d& center, double radius);

    void run_step(World& world, const FluidRules& rules, const vector3d& center, double radius);

    std::vector<BlockPos>                             m_pending;
    std::unordered_set<BlockPos, BlockPosHash>        m_pending_set;
    std::vector<BlockPos>                             m_current;
    std::vector<BlockPos>                             m_dormant;
    double                                            m_time    = 0.0;
    double                                            m_recheck = 0.0;
    FluidStats                                        m_stats;
    std::unordered_map<BlockPos, float, BlockPosHash> m_speeds;
    std::vector<FluidSplash>                          m_splashes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_FLUID_SIMULATOR_HPP