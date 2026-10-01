#ifndef VOXELSPIRE_PHYSICS_FLUID_FLOW_HPP
#define VOXELSPIRE_PHYSICS_FLUID_FLOW_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include "../core/types.hpp"

namespace voxelspire {

class FluidAccess {
public:
    virtual ~FluidAccess() = default;

    virtual bool         same(const BlockPos& p) const = 0;
    virtual bool         open(const BlockPos& p) const = 0;
    virtual std::uint8_t state(const BlockPos& p) const = 0;
    virtual void         put(const BlockPos& p, std::uint8_t state) = 0;
    virtual void         clear(const BlockPos& p) = 0;
};

class FluidRules {
public:
    static constexpr std::uint8_t FULL = 0;

    virtual ~FluidRules() = default;

    virtual std::string id() const = 0;
    virtual double interval() const noexcept = 0;
    virtual double level(std::uint8_t state) const noexcept = 0;
    virtual bool   falling(std::uint8_t) const noexcept { return false; }
    virtual bool   flows() const noexcept { return true; }
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

    std::string id() const override { return "voxelspire:still"; }
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
        : m_interval(vmax(interval, MIN_INTERVAL)), m_spread(vclamp(spread, 1, MAX_SPREAD)), m_search(vclamp(slope_search, 0, MAX_SEARCH)), m_infinite(infinite_sources) {}

    std::string id() const override { return "voxelspire:minecraft"; }
    double interval() const noexcept override { return m_interval; }

    double level(std::uint8_t state) const noexcept override {
        const int d = falling(state) ? 0 : vmin(static_cast<int>(state & DISTANCE_MASK), m_spread);
        return static_cast<double>(m_spread + 1 - d) / static_cast<double>(m_spread + 2);
    }

    bool falling(std::uint8_t state) const noexcept override { return (state & FALLING) != 0; }

    void update(FluidAccess& f, const BlockPos& p) const override {
        if (!f.same(p)) return;
        std::uint8_t s = f.state(p);

        if (s != FULL) {
            const int expected = expected_state(f, p);
            if (expected < 0) { f.clear(p); return; }
            if (expected != s) { s = static_cast<std::uint8_t>(expected); f.put(p, s); }
        }

        const BlockPos below = offset(p, DOWN);

        if (f.open(below)) {
            f.put(below, FALLING);
            return;
        }

        if (f.same(below) && f.state(below) != FULL) return;
        const int next = (falling(s) ? 0 : static_cast<int>(s & DISTANCE_MASK)) + 1;
        if (next > m_spread) return;
        const std::uint8_t mask = preferred_sides(f, p);

        for (std::size_t i = 0; i < SIDES.size(); ++i) {
            if (!(mask & (1u << i))) continue;
            const BlockPos q = offset(p, SIDES[i]);

            if (f.open(q)) { f.put(q, static_cast<std::uint8_t>(next)); continue; }
            if (!f.same(q)) continue;
            const std::uint8_t qs = f.state(q);
            if (qs == FULL || falling(qs)) continue;
            if (static_cast<int>(qs & DISTANCE_MASK) > next) f.put(q, static_cast<std::uint8_t>(next));
        }
    }

private:
    static constexpr double MIN_INTERVAL = 0.01;
    static constexpr int    MAX_SEARCH   = 8;

    int expected_state(FluidAccess& f, const BlockPos& p) const {
        if (f.same(offset(p, UP))) return FALLING;
        int nearest = UNREACHABLE, sources = 0;

        for (const BlockPos& d : SIDES) {
            const BlockPos q = offset(p, d);
            if (!f.same(q)) continue;
            const std::uint8_t qs = f.state(q);
            if (qs == FULL) ++sources;
            const int dq = qs == FULL || falling(qs) ? 0 : static_cast<int>(qs & DISTANCE_MASK);
            nearest = vmin(nearest, dq);
        }

        const BlockPos below = offset(p, DOWN);
        const bool supported = !f.open(below) && (!f.same(below) || f.state(below) == FULL);
        if (m_infinite && sources >= SOURCES_FOR_NEW && supported) return FULL;
        if (nearest >= UNREACHABLE) return -1;
        const int d = nearest + 1;
        return d > m_spread ? -1 : d;
    }

    bool passable(FluidAccess& f, const BlockPos& q) const {
        return f.open(q) || (f.same(q) && f.state(q) != FULL);
    }

    bool drops(FluidAccess& f, const BlockPos& q) const {
        const BlockPos below = offset(q, DOWN);
        return f.open(below) || (f.same(below) && f.state(below) != FULL);
    }

    int distance_to_drop(FluidAccess& f, const BlockPos& start, int from_side, int depth) const {
        if (drops(f, start)) return depth;
        if (depth >= m_search) return UNREACHABLE;
        int best = UNREACHABLE;

        for (std::size_t i = 0; i < SIDES.size(); ++i) {
            if (static_cast<int>(i) == (from_side ^ 1)) continue;
            const BlockPos q = offset(start, SIDES[i]);
            if (!passable(f, q)) continue;
            best = vmin(best, distance_to_drop(f, q, static_cast<int>(i), depth + 1));
        }

        return best;
    }

    std::uint8_t preferred_sides(FluidAccess& f, const BlockPos& p) const {
        std::array<int, 4> dist{};
        int best = UNREACHABLE;

        for (std::size_t i = 0; i < SIDES.size(); ++i) {
            const BlockPos q = offset(p, SIDES[i]);
            dist[i] = passable(f, q) ? distance_to_drop(f, q, static_cast<int>(i), 1) : UNREACHABLE;
            best = vmin(best, dist[i]);
        }

        std::uint8_t mask = 0;

        for (std::size_t i = 0; i < SIDES.size(); ++i) {
            const bool reach = passable(f, offset(p, SIDES[i]));
            if (reach && (best >= UNREACHABLE || dist[i] == best)) mask |= static_cast<std::uint8_t>(1u << i);
        }

        return mask;
    }

    double m_interval;
    int    m_spread, m_search;
    bool   m_infinite;
};

class RealisticFluid final : public FluidRules {
public:
    static constexpr double DEFAULT_INTERVAL  = 0.1;
    static constexpr double DEFAULT_MIN_DEPTH = 1.0 / 16.0;
    static constexpr int    UNITS             = 255;
    static constexpr int    SETTLE_GAP        = 2;

    explicit RealisticFluid(double interval = DEFAULT_INTERVAL, double min_depth = DEFAULT_MIN_DEPTH) noexcept
        : m_interval(vmax(interval, MIN_INTERVAL)), m_min_units(vclamp(static_cast<int>(min_depth * UNITS + 0.5), 1, UNITS)) {}

    std::string id() const override { return "voxelspire:realistic"; }
    double interval() const noexcept override { return m_interval; }
    double level(std::uint8_t state) const noexcept override { return static_cast<double>(units(state)) / UNITS; }

    static int units(std::uint8_t state) noexcept { return UNITS - static_cast<int>(state); }
    static std::uint8_t state_for(int units_left) noexcept { return static_cast<std::uint8_t>(UNITS - vclamp(units_left, 0, UNITS)); }

    void update(FluidAccess& f, const BlockPos& p) const override {
        if (!f.same(p)) return;
        int mass = units(f.state(p));
        const BlockPos below = offset(p, DOWN);
        const int room = f.open(below) ? UNITS : (f.same(below) ? UNITS - units(f.state(below)) : 0);

        if (room > 0) {
            const int move = vmin(mass, room);
            const int under = f.open(below) ? 0 : units(f.state(below));
            f.put(below, state_for(under + move));
            mass -= move;
            store(f, p, mass);
            if (mass == 0) return;
        }

        if (f.same(offset(p, UP))) return;
        spread(f, p, mass);
    }

private:
    static constexpr double MIN_INTERVAL = 0.01;

    struct Side { BlockPos pos; int mass; };

    static void store(FluidAccess& f, const BlockPos& p, int mass) {
        if (mass <= 0) f.clear(p);
        else f.put(p, state_for(mass));
    }

    void spread(FluidAccess& f, const BlockPos& p, int mass) const {
        if (mass <= m_min_units) return;
        std::array<Side, 4> sides{};
        int count = 0;

        for (const BlockPos& d : SIDES) {
            const BlockPos q = offset(p, d);
            const int qm = f.open(q) ? 0 : (f.same(q) && !f.same(offset(q, UP)) ? units(f.state(q)) : -1);
            if (qm < 0 || mass - qm < SETTLE_GAP) continue;
            sides[static_cast<std::size_t>(count++)] = { q, qm };
        }

        if (count == 0) return;
        std::sort(sides.begin(), sides.begin() + count, [](const Side& a, const Side& b) { return a.mass < b.mass; });
        int used = count, share = 0;

        for (; used > 0; --used) {
            int sum = mass;
            for (int i = 0; i < used; ++i) sum += sides[static_cast<std::size_t>(i)].mass;
            share = sum / (used + 1);
            if (share >= m_min_units && share > sides[static_cast<std::size_t>(used - 1)].mass) break;
        }

        if (used == 0) return;
        int given = 0;

        for (int i = 0; i < used; ++i) {
            const Side& s = sides[static_cast<std::size_t>(i)];
            given += share - s.mass;
            f.put(s.pos, state_for(share));
        }

        store(f, p, mass - given);
    }

    double m_interval;
    int    m_min_units;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_FLUID_FLOW_HPP