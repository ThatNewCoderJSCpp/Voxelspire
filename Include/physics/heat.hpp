#ifndef VOXELSPIRE_PHYSICS_HEAT_HPP
#define VOXELSPIRE_PHYSICS_HEAT_HPP

#include <cmath>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>
#include "../core/identifier.hpp"
#include "../core/settings.hpp"
#include "../lighting/dynamic_light.hpp"
#include "../world/world.hpp"

namespace voxelspire {

class Entity;

struct HeatSource {
    vector3d position{};
    double   heat = 0.0;
};

struct HeatGear {
    double insulation = 0.0;
    double warmth     = 0.0;
    double cooling    = 0.0;
    double speed      = 1.0;

    HeatGear& operator+=(const HeatGear& o) noexcept {
        insulation += o.insulation;
        warmth     += o.warmth;
        cooling    += o.cooling;
        speed      *= o.speed;
        return *this;
    }
};

class HeatGearRegistry {
public:
    using Source = std::function<HeatGear(const Entity&)>;

    void add(Identifier id, Source source) {
        remove(id);
        m_sources.emplace_back(std::move(id), std::move(source));
    }

    bool remove(const Identifier& id) {
        for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
            if (it->first != id) continue;
            m_sources.erase(it);
            return true;
        }

        return false;
    }

    HeatGear total(const Entity& e, HeatGear base) const {
        for (const auto& s : m_sources) base += s.second(e);
        return base;
    }

    std::size_t size() const noexcept { return m_sources.size(); }

private:
    std::vector<std::pair<Identifier, Source>> m_sources;
};

struct HeatEnvironment {
    double air          = 0.0;
    double sources      = 0.0;
    double wind         = 0.0;
    double rain         = 0.0;
    double open_sky     = 1.0;
    double submerged    = 0.0;
    bool   touching     = false;
    double ground       = 0.0;
    double conductivity = 1.0;
};

struct HeatReading {
    double air      = 0.0;
    double sources  = 0.0;
    double surround = 0.0;
    double exchange = 1.0;
    double felt     = 0.0;
};

enum class BodyState : std::uint8_t { Comfortable = 0, Cold, Freezing, Hot, Overheating };

inline const char* body_state_name(BodyState s) noexcept {
    switch (s) {
        case BodyState::Comfortable: return "comfortable";
        case BodyState::Cold:        return "cold";
        case BodyState::Freezing:    return "freezing";
        case BodyState::Hot:         return "hot";
        case BodyState::Overheating: return "overheating";
    }

    return "";
}

class HeatSensor {
public:
    static constexpr double SCAN_INTERVAL = 0.25;
    static constexpr double MIN_DISTANCE  = 1.0;
    static constexpr double FADE_START    = 0.7;
    static constexpr double RAY_STEP      = 0.5;
    static constexpr double MAX_REACH     = 32.0;

    static double reach_of(double heat, const HeatSettings& s) noexcept {
        if (s.model == HeatModel::Simple) return s.simple_radius;
        return vmin(std::sqrt(std::fabs(heat) * s.source_strength / vmax(s.faintest_warmth, 1e-6)), MAX_REACH);
    }

    static double strongest(const World& world) noexcept {
        const BlockRegistry& blocks = world.blocks();
        const BlockTraits* traits = blocks.traits_table();
        double best = 0.0;
        for (std::size_t i = 0; i < blocks.size(); ++i) best = vmax(best, std::fabs(traits[i].thermal.heat));
        return best;
    }

    void scan(const World& world, const vector3d& center, const HeatSettings& s) {
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

    double warmth(const World& world, const vector3d& at, const std::vector<DynamicLight>& lights, const HeatSettings& s) const {
        if (s.model == HeatModel::Off) return 0.0;
        double total = 0.0;
        for (const HeatSource& b : m_blocks) total += contribution(world, at, b.position, b.heat, s);
        for (const DynamicLight& l : lights) if (l.heat != 0.0) total += contribution(world, at, l.position, l.heat, s);
        return total * s.source_strength;
    }

    static double measure(const World& world, const vector3d& at, const std::vector<DynamicLight>& lights, const HeatSettings& s) {
        HeatSensor sensor;
        sensor.scan(world, at, s);
        return sensor.warmth(world, at, lights, s);
    }

    std::size_t sources() const noexcept { return m_blocks.size(); }

private:
    static double contribution(const World& world, const vector3d& at, const vector3d& from, double heat, const HeatSettings& s) {
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

    static double smoothstep(double a, double b, double v) noexcept {
        const double t = vclamp((v - a) / (b - a), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    static double passes(const World& world, const vector3d& at, const vector3d& from, const HeatSettings& s) {
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

    std::vector<HeatSource> m_blocks;
};

class BodyHeat {
public:
    static constexpr double SECONDS_PER_MINUTE = 60.0;
    static constexpr double WATER_MIN          = 1.0;
    static constexpr double WATER_MAX          = 30.0;
    static constexpr double SIMPLE_RECOVERY    = 2.0;
    static constexpr double SIMPLE_HALF        = 0.5;
    static constexpr double BODY_MIN           = 20.0;
    static constexpr double BODY_MAX           = 45.0;
    static constexpr double HALF               = 0.5;

    void reset(const HeatSettings& s) noexcept {
        m_body  = s.normal_body;
        m_meter = 0.0;
    }

    static double clothing_scale(const HeatGear& gear) noexcept {
        return (1.0 + CharacterSettings::NORMAL_CLOTHING) / (1.0 + vmax(gear.insulation, 0.0));
    }

    static HeatReading read(const HeatEnvironment& e, const HeatGear& gear, const HeatSettings& s) noexcept {
        HeatReading r;
        r.air     = e.air;
        r.sources = e.sources;

        if (s.model != HeatModel::Realistic) {
            r.surround = e.air + e.sources;
            r.felt     = r.surround;
            return r;
        }

        const double sub      = vclamp(e.submerged, 0.0, 1.0);
        const double air_w    = (1.0 - sub) * (1.0 + (s.wind_exchange * e.wind + s.rain_exchange * e.rain) * e.open_sky);
        const double water_w  = sub * s.water_exchange;
        const double ground_w = e.touching ? s.ground_exchange * e.conductivity : 0.0;
        const double total    = air_w + water_w + ground_w;
        const double water    = vclamp(e.air, WATER_MIN, WATER_MAX);
        r.surround = total > 0.0 ? (air_w * (e.air + e.sources) + water_w * water + ground_w * e.ground) / total : e.air;
        r.exchange = total;
        const double neutral   = HALF * (s.comfort_low + s.comfort_high);
        const double deviation = r.surround - neutral;
        const double clothes   = clothing_scale(gear);
        r.felt = neutral + deviation * total * (deviation < 0.0 ? clothes : 1.0 / clothes);
        return r;
    }

    void update(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear = {}) noexcept {
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

    double body()  const noexcept { return m_body; }
    double meter() const noexcept { return m_meter; }

    BodyState state(const HeatSettings& s) const noexcept {
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

    double speed_factor(const HeatSettings& s) const noexcept {
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

private:
    void simple(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept {
        const double shift = (gear.insulation - CharacterSettings::NORMAL_CLOTHING) * s.clothing_degrees;
        const double step  = dt / vmax(s.simple_seconds, 1e-3);
        if (r.felt < s.simple_cold - shift) m_meter -= step;
        else if (r.felt > s.simple_hot - shift) m_meter += step;
        else m_meter += vclamp(-m_meter, -step * SIMPLE_RECOVERY, step * SIMPLE_RECOVERY);
        m_meter = vclamp(m_meter, -1.0, 1.0);
        m_body = s.normal_body + (m_meter < 0.0 ? m_meter * (s.normal_body - s.freezing_body) : m_meter * (s.overheating_body - s.normal_body));
    }

    void realistic(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept {
        const double minutes = dt / SECONDS_PER_MINUTE;

        if (r.felt < s.comfort_low) {
            m_body -= s.exchange_rate * (s.comfort_low - r.felt) * minutes;
        } else if (r.felt > s.comfort_high) {
            m_body += s.exchange_rate * (r.felt - s.comfort_high) * minutes;
        } else {
            const double back = s.recovery_rate * minutes;
            m_body += vclamp(s.normal_body - m_body, -back, back);
        }

        m_body += (gear.warmth - gear.cooling) * minutes;
        m_body  = vclamp(m_body, BODY_MIN, BODY_MAX);
        m_meter = 0.0;
    }

    double m_body  = HeatSettings{}.normal_body;
    double m_meter = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_HEAT_HPP