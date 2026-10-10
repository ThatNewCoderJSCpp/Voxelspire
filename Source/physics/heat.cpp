#include "physics/heat.hpp"

namespace voxelspire {

HeatGear& HeatGear::operator+=(const HeatGear& o) noexcept {
    cold_degrees += o.cold_degrees;
    hot_degrees  += o.hot_degrees;
    wind_block    = layered(wind_block, o.wind_block);
    rain_block    = layered(rain_block, o.rain_block);
    water_block   = layered(water_block, o.water_block);
    warmth       += o.warmth;
    cooling      += o.cooling;
    speed        *= o.speed;
    return *this;
}

bool HeatGear::any() const noexcept {
    return cold_degrees != 0.0 || hot_degrees != 0.0 || wind_block > 0.0 || rain_block > 0.0 || water_block > 0.0 || warmth != 0.0 || cooling != 0.0;
}

bool HeatGearRegistry::remove(const Identifier& id) {
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
        if (it->first != id) continue;
        m_sources.erase(it);
        return true;
    }

    return false;
}

const char* body_state_name(BodyState s) noexcept {
    switch (s) {
        case BodyState::Comfortable: return "comfortable";
        case BodyState::Cold:        return "cold";
        case BodyState::Freezing:    return "freezing";
        case BodyState::Hot:         return "hot";
        case BodyState::Overheating: return "overheating";
    }

    return "";
}

double HeatSensor::reach_of(double heat, const HeatSettings& s) noexcept {
    if (s.model == HeatModel::Simple) return s.simple_radius;
    return vmin(std::sqrt(std::fabs(heat) * s.source_strength / vmax(s.faintest_warmth, 1e-6)), MAX_REACH);
}

double HeatSensor::strongest(const World& world) noexcept {
    const BlockRegistry& blocks = world.blocks();
    const BlockTraits* traits = blocks.traits_table();
    double best = 0.0;
    for (std::size_t i = 0; i < blocks.size(); ++i) best = vmax(best, std::fabs(traits[i].thermal.heat));
    return best;
}

void HeatSensor::scan(const World& world, const vector3d& center, const HeatSettings& s) {
    m_blocks.clear();
    if (s.model == HeatModel::Off) return;
    const double strongest_heat = strongest(world);
    if (strongest_heat <= 0.0) return;
    const int r = static_cast<int>(std::ceil(reach_of(strongest_heat, s)));
    const BlockPos c = BlockPos::containing(center);

    for (int z = c.z - r; z <= c.z + r; ++z)
        for (int y = c.y - r; y <= c.y + r; ++y)
            for (int x = c.x - r; x <= c.x + r; ++x) {
                const BlockPos p{ x, y, z };
                const double heat = world.traits_at(p).thermal.heat;
                if (heat != 0.0) m_blocks.push_back({ p.center(), heat });
            }
}

double HeatSensor::warmth(const World& world, const vector3d& at, const std::vector<DynamicLight>& lights, const HeatSettings& s) const {
    if (s.model == HeatModel::Off) return 0.0;
    double total = 0.0;
    for (const HeatSource& b : m_blocks) total += contribution(world, at, b.position, b.heat, s);
    for (const DynamicLight& l : lights) if (l.heat != 0.0) total += contribution(world, at, l.position, l.heat, s);
    return total * s.source_strength;
}

double HeatSensor::contribution(const World& world, const vector3d& at, const vector3d& from, double heat, const HeatSettings& s) {
    const double d = (from - at).magnitude();

    if (s.model == HeatModel::Simple) {
        if (d > s.simple_radius) return 0.0;
        return s.simple_falloff ? heat * (1.0 - d / s.simple_radius) : heat;
    }

    const double reach = reach_of(heat, s);
    if (d > reach) return 0.0;
    const double near = vmax(d, MIN_DISTANCE);
    const double edge = smoothstep(reach, reach * FADE_START, d);
    return heat / (near * near) * edge * passes(world, at, from, s);
}

double HeatSensor::passes(const World& world, const vector3d& at, const vector3d& from, const HeatSettings& s) {
    const vector3d step = from - at;
    const double length = step.magnitude();
    if (length <= RAY_STEP) return 1.0;
    const int steps = static_cast<int>(length / RAY_STEP);
    const BlockPos start = BlockPos::containing(at), end = BlockPos::containing(from);
    BlockPos last = start;
    double pass = 1.0;

    for (int i = 1; i < steps; ++i) {
        const BlockPos p = BlockPos::containing(at + step * (static_cast<double>(i) / steps));
        if (p == last || p == end) continue;
        last = p;
        pass *= 1.0 - vclamp(world.traits_at(p).thermal.insulation * s.insulation_scale, 0.0, 1.0);
    }

    return pass;
}

HeatReading BodyHeat::read(const HeatEnvironment& e, const HeatGear& gear, const HeatSettings& s) noexcept {
    HeatReading r;
    r.air     = e.air;
    r.sources = e.sources;

    if (s.model != HeatModel::Realistic) {
        r.surround = e.air + e.sources;
        r.felt     = r.surround;
        return r;
    }

    const double sub      = vclamp(e.submerged, 0.0, 1.0);
    const double wind     = s.wind_exchange * e.wind * (1.0 - gear.wind_block);
    const double rain     = s.rain_exchange * e.rain * (1.0 - gear.rain_block);
    const double air_w    = (1.0 - sub) * (1.0 + (wind + rain) * e.open_sky);
    const double water_w  = sub * s.water_exchange * (1.0 - gear.water_block);
    const double ground_w = e.touching ? s.ground_exchange * e.conductivity : 0.0;
    const double total    = air_w + water_w + ground_w;
    const double water    = vclamp(e.air, WATER_MIN, WATER_MAX);
    r.surround = total > 0.0 ? (air_w * (e.air + e.sources) + water_w * water + ground_w * e.ground) / total : e.air;
    r.exchange = total;
    const double neutral   = HALF * (s.comfort_low + s.comfort_high);
    const double deviation = r.surround - neutral;
    r.felt = neutral + deviation * total;
    return r;
}

void BodyHeat::update(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept {
    if (dt <= 0.0) return;

    if (!s.affects_player) {
        reset(s);
        return;
    }

    switch (s.model) {
        case HeatModel::Off:       reset(s); return;
        case HeatModel::Simple:    simple(dt, s, r, gear); return;
        case HeatModel::Realistic: realistic(dt, s, r, gear); return;
    }
}

BodyState BodyHeat::state(const HeatSettings& s) const noexcept {
    if (s.model == HeatModel::Off || !s.affects_player) return BodyState::Comfortable;

    if (s.model == HeatModel::Simple) {
        if (m_meter <= -1.0) return BodyState::Freezing;
        if (m_meter < -SIMPLE_HALF) return BodyState::Cold;
        if (m_meter >= 1.0) return BodyState::Overheating;
        if (m_meter > SIMPLE_HALF) return BodyState::Hot;
        return BodyState::Comfortable;
    }

    if (m_body <= s.freezing_body) return BodyState::Freezing;
    if (m_body <= s.cold_body) return BodyState::Cold;
    if (m_body >= s.overheating_body) return BodyState::Overheating;
    if (m_body >= s.hot_body) return BodyState::Hot;
    return BodyState::Comfortable;
}

double BodyHeat::speed_factor(const HeatSettings& s) const noexcept {
    if (!s.effects) return 1.0;

    switch (state(s)) {
        case BodyState::Comfortable: return 1.0;
        case BodyState::Cold:        return s.cold_speed;
        case BodyState::Freezing:    return s.freezing_speed;
        case BodyState::Hot:         return s.hot_speed;
        case BodyState::Overheating: return s.overheating_speed;
    }

    return 1.0;
}

void BodyHeat::simple(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept {
    const double step  = dt / vmax(s.simple_seconds, 1e-3);
    if (r.felt < s.simple_cold - gear.cold_degrees) m_meter -= step;
    else if (r.felt > s.simple_hot - gear.hot_degrees) m_meter += step;
    else m_meter += vclamp(-m_meter, -step * SIMPLE_RECOVERY, step * SIMPLE_RECOVERY);
    m_meter = vclamp(m_meter, -1.0, 1.0);
    m_body = s.normal_body + (m_meter < 0.0 ? m_meter * (s.normal_body - s.freezing_body) : m_meter * (s.overheating_body - s.normal_body));
}

void BodyHeat::realistic(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept {
    const double minutes = dt / SECONDS_PER_MINUTE;

    const double low  = s.comfort_low - gear.cold_degrees;
    const double high = s.comfort_high - gear.hot_degrees;

    if (r.felt < low) {
        m_body -= s.exchange_rate * (low - r.felt) * minutes;
    } else if (r.felt > high) {
        m_body += s.exchange_rate * (r.felt - high) * minutes;
    } else {
        const double back = s.recovery_rate * minutes;
        m_body += vclamp(s.normal_body - m_body, -back, back);
    }

    m_body += (gear.warmth - gear.cooling) * minutes;
    m_body  = vclamp(m_body, BODY_MIN, BODY_MAX);
    m_meter = 0.0;
}

} // namespace voxelspire
