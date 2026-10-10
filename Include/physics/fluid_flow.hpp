#ifndef VOXELSPIRE_PHYSICS_FLUID_FLOW_HPP
#define VOXELSPIRE_PHYSICS_FLUID_FLOW_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include "../core/identifier.hpp"
#include "../core/types.hpp"

namespace voxelspire {

struct FluidOccupancy {
    static constexpr double MAX_AREA = 0.9;
    static constexpr double STEPS    = 16.0;

    double area   = 0.0;
    double bottom = 0.0;
    double top    = 0.0;

    static double snap(double v) noexcept { return std::round(vclamp(v, 0.0, 1.0) * STEPS) / STEPS; }

    static FluidOccupancy of(double area, double bottom, double top) noexcept {
        return { vmin(snap(area), MAX_AREA), snap(bottom), snap(top) };
    }

    bool   empty()    const noexcept { return area <= 0.0 || top <= bottom; }
    double volume()   const noexcept { return empty() ? 0.0 : area * (top - bottom); }
    double capacity() const noexcept { return 1.0 - volume(); }

    double level(double water) const noexcept;

    double water_for(double level) const noexcept;

    void merge(const FluidOccupancy& o) noexcept;

    bool operator==(const FluidOccupancy& o) const noexcept { return area == o.area && bottom == o.bottom && top == o.top; }
    bool operator!=(const FluidOccupancy& o) const noexcept { return !(*this == o); }
};

struct FluidDisplacer {
    AABB   box;
    double share = 1.0;
};

struct FluidCell {
    bool         open  = false;
    bool         same  = false;
    std::uint8_t state = 0;
};

class FluidAccess {
public:
    virtual ~FluidAccess() = default;

    virtual bool           same(const BlockPos& p) const = 0;
    virtual bool           open(const BlockPos& p) const = 0;
    virtual std::uint8_t   state(const BlockPos& p) const = 0;
    virtual void           put(const BlockPos& p, std::uint8_t state) = 0;
    virtual void           clear(const BlockPos& p) = 0;
    virtual FluidOccupancy occupancy(const BlockPos&) const { return {}; }
    virtual double         speed(const BlockPos&) const { return 0.0; }
    virtual void           set_speed(const BlockPos&, double) {}
    virtual void           splash(const BlockPos&, double) {}

    virtual FluidCell look(const BlockPos& p) const {
        const bool fluid = same(p);
        return { !fluid && open(p), fluid, fluid ? state(p) : std::uint8_t(0) };
    }
};

class FluidRules {
public:
    static constexpr std::uint8_t FULL = 0;

    virtual ~FluidRules() = default;

    virtual Identifier id() const = 0;
    virtual double interval() const noexcept = 0;
    virtual double level(std::uint8_t state) const noexcept = 0;
    virtual bool   falling(std::uint8_t) const noexcept { return false; }
    virtual bool   flows() const noexcept { return true; }
    virtual bool   displaces() const noexcept { return false; }
    virtual void   update(FluidAccess& f, const BlockPos& p) const = 0;

protected:
    static constexpr std::array<BlockPos, 4> SIDES{ BlockPos{ 1, 0, 0 }, BlockPos{ -1, 0, 0 }, BlockPos{ 0, 1, 0 }, BlockPos{ 0, -1, 0 } };
    static constexpr BlockPos DOWN{ 0, 0, -1 };
    static constexpr BlockPos UP{ 0, 0, 1 };

    static BlockPos offset(const BlockPos& p, const BlockPos& d) noexcept { return { p.x + d.x, p.y + d.y, p.z + d.z }; }
};

class StillFluid final : public FluidRules {
public:
    static constexpr double IDLE_INTERVAL = 1.0;

    Identifier id() const override { return core_id(Kind::Physics, { "fluid_flow", "still" }); }
    double interval() const noexcept override { return IDLE_INTERVAL; }
    double level(std::uint8_t) const noexcept override { return 1.0; }
    bool   flows() const noexcept override { return false; }
    void   update(FluidAccess&, const BlockPos&) const override {}
};

class MinecraftFluid final : public FluidRules {
public:
    static constexpr double       DEFAULT_INTERVAL     = 0.25;
    static constexpr int          DEFAULT_SPREAD       = 7;
    static constexpr int          DEFAULT_SLOPE_SEARCH = 4;
    static constexpr int          MAX_SPREAD           = 7;
    static constexpr std::uint8_t FALLING              = 0x80;
    static constexpr std::uint8_t DISTANCE_MASK        = 0x07;
    static constexpr int          SOURCES_FOR_NEW      = 2;
    static constexpr int          UNREACHABLE          = 1 << 20;

    explicit MinecraftFluid(double interval = DEFAULT_INTERVAL, int spread = DEFAULT_SPREAD, int slope_search = DEFAULT_SLOPE_SEARCH, bool infinite_sources = true) noexcept
;

    Identifier id() const override { return core_id(Kind::Physics, { "fluid_flow", "minecraft" }); }
    double interval() const noexcept override { return m_interval; }

    double level(std::uint8_t state) const noexcept override;

    bool falling(std::uint8_t state) const noexcept override { return (state & FALLING) != 0; }

    void update(FluidAccess& f, const BlockPos& p) const override;

private:
    static constexpr double MIN_INTERVAL = 0.01;
    static constexpr int    MAX_SEARCH   = 8;

    int expected_state(FluidAccess& f, const BlockPos& p) const;

    bool passable(FluidAccess& f, const BlockPos& q) const {
        return f.open(q) || (f.same(q) && f.state(q) != FULL);
    }

    bool drops(FluidAccess& f, const BlockPos& q) const;

    int distance_to_drop(FluidAccess& f, const BlockPos& start, int from_side, int depth) const;

    std::uint8_t preferred_sides(FluidAccess& f, const BlockPos& p) const;

    double m_interval;
    int    m_spread, m_search;
    bool   m_infinite;
};

class RealisticFluid final : public FluidRules {
public:
    static constexpr double DEFAULT_INTERVAL    = 0.1;
    static constexpr double DEFAULT_MIN_DEPTH   = 1.0 / 32.0;
    static constexpr int    DEFAULT_DROP_SEARCH = 5;
    static constexpr int    MAX_DROP_SEARCH     = 8;
    static constexpr int    UNITS               = 255;
    static constexpr int    SETTLE_GAP          = 2;
    static constexpr int    NO_SEA              = std::numeric_limits<int>::min();

    static constexpr double SPLASH_FALL      = 3.0;
    static constexpr double SPILL_FALL       = 6.0;
    static constexpr double SPLASH_SHARE     = 0.06;
    static constexpr double MAX_SPLASH_SHARE = 0.6;
    static constexpr double SPEED_KEEP       = 0.5;
    static constexpr double SPEED_DECAY      = 0.6;
    static constexpr double MIN_SPEED        = 0.5;

    explicit RealisticFluid(
        double interval    = DEFAULT_INTERVAL,
        double min_depth   = DEFAULT_MIN_DEPTH,
        bool   seek_drops  = true,
        int    drop_search = DEFAULT_DROP_SEARCH,
        bool   displace    = false,
        double splash      = 0.0,
        int    sea_level   = NO_SEA
    ) noexcept
;

    Identifier id() const override { return core_id(Kind::Physics, { "fluid_flow", "realistic" }); }
    double interval() const noexcept override { return m_interval; }
    double level(std::uint8_t state) const noexcept override { return static_cast<double>(units(state)) / UNITS; }
    bool   displaces() const noexcept override { return m_displace; }

    static int units(std::uint8_t state) noexcept { return UNITS - static_cast<int>(state); }
    static std::uint8_t state_for(int units_left) noexcept { return static_cast<std::uint8_t>(UNITS - vclamp(units_left, 0, UNITS)); }

    void update(FluidAccess& f, const BlockPos& p) const override;

private:
    void flow(FluidAccess& f, const BlockPos& p) const;

    static constexpr double MIN_INTERVAL    = 0.01;
    static constexpr int    WINDOW          = 2 * MAX_DROP_SEARCH + 1;
    static constexpr int    NO_SIDE         = -1;
    static constexpr int    MAX_PUSH_HEIGHT = 32;
    static constexpr int    THIN_FACTOR     = 4;

    struct Side { BlockPos pos; int mass; int level; FluidOccupancy occ; };

    bool open_sea(FluidAccess& f, const BlockPos& q) const {
        return q.z == m_sea && f.same(q) && f.same(offset(q, DOWN));
    }

    static bool surrounded(FluidAccess& f, const BlockPos& p) {
        for (const BlockPos& d : SIDES) if (!f.same(offset(p, d))) return false;
        return true;
    }

    bool keeps_full(FluidAccess& f, const BlockPos& p) const;

    bool sea_exchange(FluidAccess& f, const BlockPos& p) const;

    int splash(FluidAccess& f, const BlockPos& p, int mass, double speed) const;

    static void store(FluidAccess& f, const BlockPos& p, int mass) {
        if (mass <= 0) f.clear(p);
        else f.put(p, state_for(mass));
    }

    static int mass_in(FluidAccess& f, const BlockPos& q) { return f.same(q) ? units(f.state(q)) : 0; }

    FluidOccupancy occupancy(FluidAccess& f, const BlockPos& q) const { return m_displace ? f.occupancy(q) : FluidOccupancy{}; }

    static int capacity(const FluidOccupancy& occ) noexcept { return static_cast<int>(occ.capacity() * UNITS); }

    static int level_units(int mass, const FluidOccupancy& occ) noexcept;

    static int mass_for(int level, const FluidOccupancy& occ) noexcept;

    int room_in(FluidAccess& f, const BlockPos& q) const {
        if (!f.open(q) && !f.same(q)) return 0;
        return vmax(capacity(occupancy(f, q)) - mass_in(f, q), 0);
    }

    int give(FluidAccess& f, const BlockPos& q, int amount) const;

    int overflow(FluidAccess& f, const BlockPos& p, int mass) const;

    BlockPos surface_above(FluidAccess& f, const BlockPos& p) const;

    int spread(FluidAccess& f, const BlockPos& p, int mass) const;

    void drain(FluidAccess& f, const BlockPos& p, int mass) const;

    int room_of(FluidAccess& f, const BlockPos& q, const FluidCell& c) const {
        if (!c.open && !c.same) return 0;
        return capacity(occupancy(f, q)) - (c.same ? units(c.state) : 0);
    }

    bool drops(FluidAccess& f, const BlockPos& q, const FluidCell& c) const;

    int drop_side(FluidAccess& f, const BlockPos& p) const;

    double m_interval;
    int    m_min_units;
    int    m_search;
    bool   m_displace;
    double m_splash;
    int    m_sea;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_FLUID_FLOW_HPP