#ifndef VOXELSPIRE_ENTITY_MOVEMENT_MODE_HPP
#define VOXELSPIRE_ENTITY_MOVEMENT_MODE_HPP

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>
#include "character_settings.hpp"
#include "../core/function_ref.hpp"
#include "../core/types.hpp"

namespace voxelspire {

class World;

struct MovementIntent {
    double forward = 0.0;
    double strafe  = 0.0;
    bool   jump    = false;
    bool   sprint  = false;
    bool   crouch  = false;
    bool   crawl   = false;
    bool   swim    = false;
    bool   fly     = false;
    bool   alt     = false;
};

struct MovementStats {
    double speed         = 0.0;
    double ground_accel  = 0.0;
    double air_accel     = 0.0;
    double jump_velocity = 0.0;
    double gravity_scale = 1.0;
    double drag_scale    = 1.0;
};

struct MovementContext {
    const MovementIntent&               intent;
    const World&                        world;
    bool                                on_ground;
    bool                                in_fluid;
    FunctionRef<bool(Identifier pose)>  fits;
    double                              vertical_velocity = 0.0;
    double                              fall_distance     = 0.0;
    FunctionRef<double(double)>         ground_distance;
    bool                                head_in_fluid     = false;
    vector3d                            look{};
    vector3d                            right{};
};

namespace MovementModes {
    inline const Identifier Walk    { "voxelspire:walk" };
    inline const Identifier Sprint  { "voxelspire:sprint" };
    inline const Identifier Crouch  { "voxelspire:crouch" };
    inline const Identifier Crawl   { "voxelspire:crawl" };
    inline const Identifier Swim    { "voxelspire:swim" };
    inline const Identifier Stroke  { "voxelspire:swim_stroke" };
    inline const Identifier Flutter { "voxelspire:flutter" };
    inline const Identifier Fly     { "voxelspire:fly" };
} // namespace MovementModes

struct MovementRatios {
    static constexpr double speed(double s) noexcept { return s / PlayerDefaults::movement::movement_speed; }
    static constexpr double acceleration(double a) noexcept { return a / PlayerDefaults::movement::ground_acceleration; }
};

class MovementMode {
public:
    MovementMode(Identifier id, int priority, Identifier pose, double speed_multiplier, double alt_speed_multiplier)
        : m_id(id), m_pose(pose), m_priority(priority), m_speed(speed_multiplier), m_alt_speed(alt_speed_multiplier) {}
    virtual ~MovementMode() = default;

    Identifier         id()               const noexcept { return m_id; }
    Identifier         pose()             const noexcept { return m_pose; }
    int                priority()         const noexcept { return m_priority; }
    double             speed_multiplier() const noexcept { return m_speed; }
    double             alt_speed_multiplier() const noexcept { return m_alt_speed; }
    double             speed_multiplier(const MovementIntent& intent) const noexcept { return intent.alt ? m_alt_speed : m_speed; }

    virtual bool   is_active(const MovementContext& ctx) const = 0;
    virtual double acceleration_multiplier() const noexcept { return 1.0; }
    virtual double gravity_scale()           const noexcept { return 1.0; }
    virtual double drag_scale(const MovementIntent&) const noexcept { return 1.0; }
    virtual bool   can_jump()                const noexcept { return true; }
    
    virtual double rising_speed_limit(const MovementStats& stats, const MovementIntent& intent) const noexcept {
        return vmax(stats.speed * speed_multiplier(intent), can_jump() ? stats.jump_velocity : 0.0);
    }

    virtual double sinking_speed_limit(const MovementStats&, const MovementIntent&) const noexcept { return 0.0; }

    virtual void update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const {
        const vector3d target = wish * (stats.speed * speed_multiplier(ctx.intent));
        const double accel = (ctx.on_ground ? stats.ground_accel : stats.air_accel) * acceleration_multiplier() * dt;
        v.x = approach(v.x, target.x, accel);
        v.y = approach(v.y, target.y, accel);
        if (can_jump() && ctx.intent.jump && ctx.on_ground) v.z = stats.jump_velocity;
    }

protected:
    static double approach(double v, double target, double max_delta) noexcept {
        const double diff = target - v;
        if (std::fabs(diff) <= max_delta) return target;
        return v + (diff > 0.0 ? max_delta : -max_delta);
    }

private:
    Identifier  m_id;
    Identifier  m_pose;
    int         m_priority;
    double      m_speed;
    double      m_alt_speed;
};

class WalkMode final : public MovementMode {
public:
    explicit WalkMode(
        double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::movement_speed),
        double alt_speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::alt_walk_speed)
    ) : MovementMode(MovementModes::Walk, 0, Poses::Standing, speed_multiplier, alt_speed_multiplier) {}
    
    bool is_active(const MovementContext&) const override { return true; }
};

class SprintMode final : public MovementMode {
public:
    explicit SprintMode(
        double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::sprint_speed),
        double alt_speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::alt_sprint_speed)
    ) : MovementMode(MovementModes::Sprint, 100, Poses::Standing, speed_multiplier, alt_speed_multiplier) {}
    
    bool is_active(const MovementContext& ctx) const override {
        return ctx.intent.sprint && ctx.intent.forward > 0.0 && ctx.fits(pose());
    }
};

class CrouchMode final : public MovementMode {
public:
    explicit CrouchMode(
        double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::crouch_speed),
        double alt_speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::alt_crouch_speed)
    ) : MovementMode(MovementModes::Crouch, 200, Poses::Crouching, speed_multiplier, alt_speed_multiplier) {}
    
    bool is_active(const MovementContext& ctx) const override {
        return ctx.intent.crouch || !ctx.fits(Poses::Standing);
    }
};

class CrawlMode final : public MovementMode {
public:
    explicit CrawlMode(
        double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::crawl_speed),
        double alt_speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::alt_crawl_speed)
    ) : MovementMode(MovementModes::Crawl, 300, Poses::Prone, speed_multiplier, alt_speed_multiplier) {}
    
    bool is_active(const MovementContext& ctx) const override {
        return ctx.intent.crawl || !ctx.fits(Poses::Crouching);
    }

    bool can_jump() const noexcept override { return false; }
};

class SwimMode final : public MovementMode {
public:
    static constexpr int DEFAULT_PRIORITY = 400;

    explicit SwimMode(
        double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::swim_speed),
        double alt_speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::alt_swim_speed),
        double accel_multiplier = MovementRatios::acceleration(PlayerDefaults::movement::swim_acceleration),
        double rise_speed = PlayerDefaults::swimming::rise_speed, 
        double sink_speed = PlayerDefaults::swimming::sink_speed,
        double vertical_accel = PlayerDefaults::swimming::vertical_accel, 
        double surface_leap = PlayerDefaults::swimming::surface_leap
    )
        : MovementMode(MovementModes::Swim, DEFAULT_PRIORITY, Poses::Standing, speed_multiplier, alt_speed_multiplier),
          m_accel(accel_multiplier), m_rise(rise_speed), m_sink(sink_speed), m_vertical_accel(vertical_accel), m_leap(surface_leap) {}

    bool   is_active(const MovementContext& ctx) const override { return ctx.in_fluid; }
    double acceleration_multiplier() const noexcept override { return m_accel; }
    double gravity_scale()           const noexcept override { return 0.0; }
    bool   can_jump()                const noexcept override { return false; }
    double rising_speed_limit(const MovementStats& stats, const MovementIntent&) const noexcept override { return vmax(m_rise, stats.jump_velocity * m_leap); }
    double sinking_speed_limit(const MovementStats&, const MovementIntent&) const noexcept override { return vmax(m_rise, m_sink); }

    void update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const override {
        const vector3d target = wish * (stats.speed * speed_multiplier(ctx.intent));
        const double accel = stats.ground_accel * acceleration_multiplier() * dt;
        v.x = approach(v.x, target.x, accel);
        v.y = approach(v.y, target.y, accel);

        if (ctx.intent.jump && !ctx.head_in_fluid && (wish.x != 0.0 || wish.y != 0.0)) {
            v.z = vmax(v.z, stats.jump_velocity * m_leap);
            return;
        }

        const double vz = ctx.intent.jump ? m_rise : (ctx.intent.crouch ? -m_rise : -m_sink);
        v.z = approach(v.z, vz, m_vertical_accel * dt);
    }

private:
    double m_accel, m_rise, m_sink, m_vertical_accel, m_leap;
};

class StrokeMode final : public MovementMode {
public:
    static constexpr int DEFAULT_PRIORITY = 450;

    explicit StrokeMode(
        double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::stroke_speed),
        double alt_speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::alt_stroke_speed),
        double accel = MovementRatios::acceleration(PlayerDefaults::movement::stroke_acceleration),
        double buoyancy = PlayerDefaults::swimming::stroke_buoyancy
    ) : MovementMode(MovementModes::Stroke, DEFAULT_PRIORITY, Poses::Swimming, speed_multiplier, alt_speed_multiplier), m_accel(accel), m_buoyancy(buoyancy) {}

    bool is_active(const MovementContext& ctx) const override {
        return ctx.in_fluid && (ctx.intent.swim || !ctx.fits(Poses::Crouching));
    }

    double gravity_scale() const noexcept override { return 0.0; }
    bool   can_jump()      const noexcept override { return false; }
    double sinking_speed_limit(const MovementStats& stats, const MovementIntent& intent) const noexcept override { return stats.speed * speed_multiplier(intent); }

    void update_velocity(vector3d& v, const vector3d&, const MovementContext& ctx, const MovementStats& stats, double dt) const override {
        vector3d dir = ctx.look * ctx.intent.forward + ctx.right * ctx.intent.strafe;
        const double len = dir.magnitude();
        if (len > 1.0) dir = dir / len;
        const vector3d target = dir * (stats.speed * speed_multiplier(ctx.intent)) + vector3d{ 0.0, 0.0, len > 0.0 ? 0.0 : m_buoyancy };
        const double accel = stats.ground_accel * m_accel * dt;
        v.x = approach(v.x, target.x, accel);
        v.y = approach(v.y, target.y, accel);
        v.z = approach(v.z, target.z, accel);
    }

private:
    double m_accel, m_buoyancy;
};

class FlutterMode final : public MovementMode {
public:
    static constexpr double DEFAULT_DRAG        = 140.0;
    static constexpr double DEFAULT_MIN_DROP    = 2.5;
    static constexpr double DEFAULT_PROBE_DEPTH = 32.0;
    static constexpr double DEFAULT_ALT_DRAG    = 35.0;
    static constexpr int    DEFAULT_PRIORITY    = 150;

    explicit FlutterMode(
        double drag = DEFAULT_DRAG, double min_drop = DEFAULT_MIN_DROP,
        double probe_depth = DEFAULT_PROBE_DEPTH, double speed_multiplier = MovementRatios::speed(PlayerDefaults::movement::movement_speed),
        double alt_drag = DEFAULT_ALT_DRAG, int priority = DEFAULT_PRIORITY
    )
        : MovementMode(MovementModes::Flutter, priority, Poses::Standing, speed_multiplier, speed_multiplier),
          m_drag(drag), m_alt_drag(alt_drag), m_min_drop(min_drop), m_probe(vmax(probe_depth, min_drop)) {}

    bool is_active(const MovementContext& ctx) const override {
        if (ctx.on_ground || ctx.in_fluid || ctx.vertical_velocity >= 0.0) return false;
        if (ctx.fall_distance >= m_min_drop) return true;
        const double remaining = ctx.ground_distance ? ctx.ground_distance(m_probe) : m_probe;
        return ctx.fall_distance + remaining >= m_min_drop;
    }

    double drag_scale(const MovementIntent& intent) const noexcept override { return intent.alt ? m_alt_drag : m_drag; }
    bool   can_jump()   const noexcept override { return false; }

    double min_drop() const noexcept { return m_min_drop; }

private:
    double m_drag, m_alt_drag, m_min_drop, m_probe;
};

class FlyMode final : public MovementMode {
public:
    static constexpr double DEFAULT_SPEED_MULTIPLIER = MovementRatios::speed(PlayerDefaults::movement::fly_speed);
    static constexpr double DEFAULT_ALT_MULTIPLIER   = MovementRatios::speed(PlayerDefaults::movement::alt_fly_speed);
    static constexpr double DEFAULT_VERTICAL_SPEED   = PlayerDefaults::flying::vertical_speed;
    static constexpr double DEFAULT_VERTICAL_ACCEL   = PlayerDefaults::flying::vertical_accel;
    static constexpr int    DEFAULT_PRIORITY         = 350;

    explicit FlyMode(
        double speed_multiplier = DEFAULT_SPEED_MULTIPLIER, 
        double vertical_speed = DEFAULT_VERTICAL_SPEED,
        double vertical_accel = DEFAULT_VERTICAL_ACCEL, 
        double alt_speed_multiplier = DEFAULT_ALT_MULTIPLIER, 
        int priority = DEFAULT_PRIORITY
    )
        : MovementMode(MovementModes::Fly, priority, Poses::Standing, speed_multiplier, alt_speed_multiplier),
          m_vertical_speed(vertical_speed), m_vertical_accel(vertical_accel) {}

    bool   is_active(const MovementContext& ctx) const override { return ctx.intent.fly && !ctx.in_fluid; }
    double gravity_scale() const noexcept override { return 0.0; }
    bool   can_jump()      const noexcept override { return false; }

    void update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const override {
        const vector3d target = wish * (stats.speed * speed_multiplier(ctx.intent));
        const double accel = stats.air_accel * dt;
        v.x = approach(v.x, target.x, accel);
        v.y = approach(v.y, target.y, accel);
        const double climb = (ctx.intent.jump ? 1.0 : 0.0) - (ctx.intent.crouch ? 1.0 : 0.0);
        v.z = approach(v.z, climb * m_vertical_speed, m_vertical_accel * dt);
    }

private:
    double m_vertical_speed, m_vertical_accel;
};

class MovementModeRegistry {
public:
    template <typename T, typename... Args>
    T& add(Args&&... args) {
        auto mode = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *mode;
        remove(ref.id());
        m_modes.push_back(std::move(mode));
        std::stable_sort(m_modes.begin(), m_modes.end(), [](const auto& a, const auto& b) { return a->priority() > b->priority(); });
        return ref;
    }

    bool remove(Identifier id) {
        auto it = std::find_if(m_modes.begin(), m_modes.end(), [&](const auto& m) { return m->id() == id; });
        if (it == m_modes.end()) return false;
        m_modes.erase(it);
        return true;
    }

    const MovementMode* find(Identifier id) const noexcept {
        for (const auto& m : m_modes) if (m->id() == id) return m.get();
        return nullptr;
    }

    const MovementMode* select(const MovementContext& ctx) const {
        for (const auto& m : m_modes) if (m->is_active(ctx)) return m.get();
        return nullptr;
    }

    std::size_t size() const noexcept { return m_modes.size(); }

    static void register_defaults(MovementModeRegistry& r, const CharacterSettings& c = CharacterSettings{}) {
        r.add<WalkMode>(c.speed_ratio(c.walk_speed), c.speed_ratio(c.alt_walk_speed));
        r.add<SprintMode>(c.speed_ratio(c.sprint_speed), c.speed_ratio(c.alt_sprint_speed));
        r.add<CrouchMode>(c.speed_ratio(c.crouch_speed), c.speed_ratio(c.alt_crouch_speed));
        r.add<CrawlMode>(c.speed_ratio(c.crawl_speed), c.speed_ratio(c.alt_crawl_speed));
        
        r.add<SwimMode>(
            c.speed_ratio(c.swim_speed), c.speed_ratio(c.alt_swim_speed), 
            c.acceleration_ratio(c.swim_acceleration),
            c.swim_rise_speed, c.swim_sink_speed, c.swim_vertical_accel, c.surface_leap
        );

        r.add<StrokeMode>(c.speed_ratio(c.stroke_speed), c.speed_ratio(c.alt_stroke_speed), c.acceleration_ratio(c.stroke_acceleration), c.stroke_buoyancy);
    }

private:
    std::vector<std::unique_ptr<MovementMode>> m_modes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_MOVEMENT_MODE_HPP