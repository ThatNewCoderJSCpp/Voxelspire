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
    double cold_degrees = 0.0;
    double hot_degrees  = 0.0;
    double wind_block   = 0.0;
    double rain_block   = 0.0;
    double water_block  = 0.0;
    double warmth       = 0.0;
    double cooling      = 0.0;
    double speed        = 1.0;

    HeatGear& operator+=(const HeatGear& o) noexcept;

    bool any() const noexcept;

    static double layered(double a, double b) noexcept { return 1.0 - (1.0 - vclamp(a, 0.0, 1.0)) * (1.0 - vclamp(b, 0.0, 1.0)); }
};

class HeatGearRegistry {
public:
    using Source = std::function<HeatGear(const Entity&)>;

    void add(Identifier id, Source source) {
        remove(id);
        m_sources.emplace_back(std::move(id), std::move(source));
    }

    bool remove(const Identifier& id);

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

const char* body_state_name(BodyState s) noexcept;

class HeatSensor {
public:
    static constexpr double SCAN_INTERVAL = 0.25;
    static constexpr double MIN_DISTANCE  = 1.0;
    static constexpr double FADE_START    = 0.7;
    static constexpr double RAY_STEP      = 0.5;
    static constexpr double MAX_REACH     = 32.0;

    static double reach_of(double heat, const HeatSettings& s) noexcept;

    static double strongest(const World& world) noexcept;

    void scan(const World& world, const vector3d& center, const HeatSettings& s);

    double warmth(const World& world, const vector3d& at, const std::vector<DynamicLight>& lights, const HeatSettings& s) const;

    static double measure(const World& world, const vector3d& at, const std::vector<DynamicLight>& lights, const HeatSettings& s) {
        HeatSensor sensor;
        sensor.scan(world, at, s);
        return sensor.warmth(world, at, lights, s);
    }

    std::size_t sources() const noexcept { return m_blocks.size(); }

private:
    static double contribution(const World& world, const vector3d& at, const vector3d& from, double heat, const HeatSettings& s);

    static double smoothstep(double a, double b, double v) noexcept {
        const double t = vclamp((v - a) / (b - a), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    static double passes(const World& world, const vector3d& at, const vector3d& from, const HeatSettings& s);

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

    static HeatReading read(const HeatEnvironment& e, const HeatGear& gear, const HeatSettings& s) noexcept;

    void update(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear = {}) noexcept;

    double body()  const noexcept { return m_body; }
    double meter() const noexcept { return m_meter; }

    BodyState state(const HeatSettings& s) const noexcept;

    double speed_factor(const HeatSettings& s) const noexcept;

private:
    void simple(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept;

    void realistic(double dt, const HeatSettings& s, const HeatReading& r, const HeatGear& gear) noexcept;

    double m_body  = HeatSettings{}.normal_body;
    double m_meter = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_PHYSICS_HEAT_HPP