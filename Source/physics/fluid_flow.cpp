#include "physics/fluid_flow.hpp"

namespace voxelspire {

double FluidOccupancy::level(double water) const noexcept {
    if (empty() || water <= bottom) return water;
    const double band = (top - bottom) * (1.0 - area);
    if (water <= bottom + band) return bottom + (water - bottom) / (1.0 - area);
    return top + (water - bottom - band);
}

double FluidOccupancy::water_for(double level) const noexcept {
    if (empty() || level <= bottom) return level;
    if (level <= top) return bottom + (level - bottom) * (1.0 - area);
    return level - volume();
}

void FluidOccupancy::merge(const FluidOccupancy& o) noexcept {
    if (o.empty()) return;
    if (empty()) { *this = o; return; }
    area   = vmin(area + o.area, MAX_AREA);
    bottom = vmin(bottom, o.bottom);
    top    = vmax(top, o.top);
}

MinecraftFluid::MinecraftFluid(double interval, int spread, int slope_search, bool infinite_sources) noexcept : m_interval(vmax(interval, MIN_INTERVAL)), m_spread(vclamp(spread, 1, MAX_SPREAD)), m_search(vclamp(slope_search, 0, MAX_SEARCH)), m_infinite(infinite_sources) {}

double MinecraftFluid::level(std::uint8_t state) const noexcept {
    const int d = falling(state) ? 0 : vmin(static_cast<int>(state & DISTANCE_MASK), m_spread);
    return static_cast<double>(m_spread + 1 - d) / static_cast<double>(m_spread + 2);
}

void MinecraftFluid::update(FluidAccess& f, const BlockPos& p) const {
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

int MinecraftFluid::expected_state(FluidAccess& f, const BlockPos& p) const {
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

bool MinecraftFluid::drops(FluidAccess& f, const BlockPos& q) const {
    const BlockPos below = offset(q, DOWN);
    return f.open(below) || (f.same(below) && f.state(below) != FULL);
}

int MinecraftFluid::distance_to_drop(FluidAccess& f, const BlockPos& start, int from_side, int depth) const {
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

std::uint8_t MinecraftFluid::preferred_sides(FluidAccess& f, const BlockPos& p) const {
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

RealisticFluid::RealisticFluid(
        double interval,
        double min_depth,
        bool   seek_drops,
        int    drop_search,
        bool   displace,
        double splash,
        int    sea_level   
) noexcept : m_interval(vmax(interval, MIN_INTERVAL)),
          m_min_units(vclamp(static_cast<int>(min_depth * UNITS + 0.5), 1, UNITS)),
          m_search(seek_drops ? vclamp(drop_search, 1, MAX_DROP_SEARCH) : 0),
          m_displace(displace),
          m_splash(vmax(splash, 0.0)),
          m_sea(sea_level) {}

void RealisticFluid::update(FluidAccess& f, const BlockPos& p) const {
    if (!f.same(p)) return;
    if (sea_exchange(f, p)) return;
    flow(f, p);
    if (keeps_full(f, p)) f.put(p, state_for(UNITS));
}

void RealisticFluid::flow(FluidAccess& f, const BlockPos& p) const {
    int mass = units(f.state(p));
    if (m_displace) mass = overflow(f, p, mass);
    if (mass == 0) return;
    const BlockPos below = offset(p, DOWN);
    const int room = room_in(f, below);
    const double speed = m_splash > 0.0 ? f.speed(p) : 0.0;

    if (room > 0) {
        const int move = vmin(mass, room);
        const bool free_fall = m_splash > 0.0 && (!f.same(below) || !surrounded(f, p));
        f.put(below, state_for(mass_in(f, below) + move));
        if (free_fall) f.set_speed(below, vmax(f.speed(below), speed + 1.0));
        mass -= move;
        store(f, p, mass);
        if (mass == 0) return;
    }

    if (speed >= SPLASH_FALL && room == 0) {
        mass = splash(f, p, mass, speed);
        if (mass == 0) return;
    } else if (speed > 0.0) {
        f.set_speed(p, speed > MIN_SPEED ? speed * SPEED_DECAY : 0.0);
    }

    const int settled = spread(f, p, mass);
    if (m_search > 0 && settled == mass && mass <= THIN_FACTOR * m_min_units) drain(f, p, mass);
}

bool RealisticFluid::keeps_full(FluidAccess& f, const BlockPos& p) const {
    if (m_sea == NO_SEA || p.z != m_sea || !f.same(offset(p, DOWN))) return false;
    if (!f.same(p) && !f.open(p)) return false;
    return mass_in(f, p) < UNITS && capacity(occupancy(f, p)) >= UNITS;
}

bool RealisticFluid::sea_exchange(FluidAccess& f, const BlockPos& p) const {
    if (m_sea == NO_SEA) return false;

    if (p.z == m_sea + 1 && open_sea(f, offset(p, DOWN))) {
        f.clear(p);
        return true;
    }

    if (open_sea(f, p) && units(f.state(p)) < UNITS && capacity(occupancy(f, p)) >= UNITS) {
        f.put(p, state_for(UNITS));
        return true;
    }

    return false;
}

int RealisticFluid::splash(FluidAccess& f, const BlockPos& p, int mass, double speed) const {
    const double energy = (speed - SPLASH_FALL + 1.0) * m_splash;
    const int spray = static_cast<int>(mass * vmin(MAX_SPLASH_SHARE, energy * SPLASH_SHARE));
    f.set_speed(p, 0.0);
    f.splash(p, energy);
    if (spray <= 0) return mass;
    const int share = vmax(spray / static_cast<int>(SIDES.size()), 1);
    int left = mass;

    for (const BlockPos& d : SIDES) {
        if (left <= share) break;
        const BlockPos q = offset(p, d);
        BlockPos target = q;

        if (!f.open(q) && !f.same(q)) {
            if (speed < SPILL_FALL) continue;
            target = offset(q, UP);
        } else {
            const BlockPos far = offset(q, d);
            if (room_in(f, far) > 0) target = far;
        }

        const int given = give(f, target, share);
        if (given <= 0) continue;
        f.set_speed(target, speed * SPEED_KEEP);
        left -= given;
    }

    store(f, p, left);
    return left;
}

int RealisticFluid::level_units(int mass, const FluidOccupancy& occ) noexcept {
    return occ.empty() ? mass : static_cast<int>(std::lround(occ.level(static_cast<double>(mass) / UNITS) * UNITS));
}

int RealisticFluid::mass_for(int level, const FluidOccupancy& occ) noexcept {
    return occ.empty() ? level : static_cast<int>(std::lround(occ.water_for(static_cast<double>(level) / UNITS) * UNITS));
}

int RealisticFluid::give(FluidAccess& f, const BlockPos& q, int amount) const {
    const int n = vmin(room_in(f, q), amount);
    if (n <= 0) return 0;
    f.put(q, state_for(mass_in(f, q) + n));
    return n;
}

int RealisticFluid::overflow(FluidAccess& f, const BlockPos& p, int mass) const {
    const int cap = capacity(f.occupancy(p));
    if (mass <= cap) return mass;
    int excess = mass - cap;
    excess -= give(f, surface_above(f, p), excess);
    for (const BlockPos& d : SIDES) if (excess > 0) excess -= give(f, offset(p, d), excess);
    const int kept = cap + excess;
    store(f, p, kept);
    return kept;
}

BlockPos RealisticFluid::surface_above(FluidAccess& f, const BlockPos& p) const {
    BlockPos q = offset(p, UP);
    for (int i = 0; i < MAX_PUSH_HEIGHT && f.same(q) && room_in(f, q) == 0; ++i) q = offset(q, UP);
    return q;
}

int RealisticFluid::spread(FluidAccess& f, const BlockPos& p, int mass) const {
    if (mass <= m_min_units) return mass;
    const FluidOccupancy own = occupancy(f, p);
    const int own_level = level_units(mass, own);
    std::array<Side, 4> sides{};
    int count = 0;

    for (const BlockPos& d : SIDES) {
        const BlockPos q = offset(p, d);
        const bool open = f.open(q);
        if (!open && !(f.same(q) && !f.same(offset(q, UP)))) continue;
        const FluidOccupancy occ = occupancy(f, q);
        const int qm = open ? 0 : units(f.state(q));
        const int ql = level_units(qm, occ);
        if (own_level - ql < SETTLE_GAP) continue;
        sides[static_cast<std::size_t>(count++)] = { q, qm, ql, occ };
    }

    if (count == 0) return mass;
    std::sort(sides.begin(), sides.begin() + count, [](const Side& a, const Side& b) { return a.level < b.level; });
    int used = count, share = 0;

    for (; used > 0; --used) {
        int sum = own_level;
        for (int i = 0; i < used; ++i) sum += sides[static_cast<std::size_t>(i)].level;
        share = sum / (used + 1);
        if (share >= m_min_units && share > sides[static_cast<std::size_t>(used - 1)].level) break;
    }

    if (used == 0) return mass;
    int left = mass - vmin(mass_for(share, own), mass);

    for (int i = 0; i < used && left > 0; ++i) {
        const Side& s = sides[static_cast<std::size_t>(i)];
        const int n = vmin(vmin(mass_for(share, s.occ), capacity(s.occ)) - s.mass, left);
        if (n <= 0) continue;
        f.put(s.pos, state_for(s.mass + n));
        mass -= n;
        left -= n;
    }

    store(f, p, mass);
    return mass;
}

void RealisticFluid::drain(FluidAccess& f, const BlockPos& p, int mass) const {
    const int side = drop_side(f, p);
    if (side == NO_SIDE) return;
    const BlockPos q = offset(p, SIDES[static_cast<std::size_t>(side)]);
    const FluidOccupancy occ = occupancy(f, q);
    const int qm = mass_in(f, q);
    const int diff = level_units(mass, occupancy(f, p)) - level_units(qm, occ);
    const int move = vmin(vmin(mass, room_in(f, q)), diff);
    if (move <= 0) return;
    f.put(q, state_for(qm + move));
    store(f, p, mass - move);
}

bool RealisticFluid::drops(FluidAccess& f, const BlockPos& q, const FluidCell& c) const {
    if (room_of(f, q, c) <= 0) return false;
    const BlockPos below = offset(q, DOWN);
    return room_of(f, below, f.look(below)) > 0;
}

int RealisticFluid::drop_side(FluidAccess& f, const BlockPos& p) const {
    std::array<std::int8_t, WINDOW * WINDOW> first;
    std::array<std::int8_t, WINDOW * WINDOW> dist{};
    std::array<int, WINDOW * WINDOW>         queue{};
    first.fill(NO_SIDE);
    auto cell = [](int dx, int dy) { return (dy + MAX_DROP_SEARCH) * WINDOW + dx + MAX_DROP_SEARCH; };
    int head = 0, tail = 0;
    first[static_cast<std::size_t>(cell(0, 0))] = static_cast<std::int8_t>(SIDES.size());

    for (std::size_t i = 0; i < SIDES.size(); ++i) {
        const BlockPos q = offset(p, SIDES[i]);
        const FluidCell look = f.look(q);
        if (!look.open && !look.same) continue;
        if (drops(f, q, look)) return static_cast<int>(i);
        const int c = cell(SIDES[i].x, SIDES[i].y);
        first[static_cast<std::size_t>(c)] = static_cast<std::int8_t>(i);
        dist[static_cast<std::size_t>(c)] = 1;
        queue[static_cast<std::size_t>(tail++)] = c;
    }

    while (head < tail) {
        const int c = queue[static_cast<std::size_t>(head++)];
        if (dist[static_cast<std::size_t>(c)] >= m_search) continue;
        const int cx = c % WINDOW - MAX_DROP_SEARCH, cy = c / WINDOW - MAX_DROP_SEARCH;

        for (const BlockPos& d : SIDES) {
            const int nx = cx + d.x, ny = cy + d.y;
            const int n = cell(nx, ny);
            if (first[static_cast<std::size_t>(n)] != NO_SIDE) continue;
            const BlockPos q{ p.x + nx, p.y + ny, p.z };
            const FluidCell look = f.look(q);
            if (!look.open && !look.same) continue;
            if (drops(f, q, look)) return first[static_cast<std::size_t>(c)];
            first[static_cast<std::size_t>(n)] = first[static_cast<std::size_t>(c)];
            dist[static_cast<std::size_t>(n)] = static_cast<std::int8_t>(dist[static_cast<std::size_t>(c)] + 1);
            queue[static_cast<std::size_t>(tail++)] = n;
        }
    }

    return NO_SIDE;
}

} // namespace voxelspire
