#ifndef VOXELSPIRE_ENTITY_MOVEMENT_MODE_HPP
#define VOXELSPIRE_ENTITY_MOVEMENT_MODE_HPP

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "entity_body.hpp"
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
};

struct MovementStats {
    double speed         = 0.0;
    double ground_accel  = 0.0;
    double air_accel     = 0.0;
    double jump_velocity = 0.0;
    double gravity_scale = 1.0;
};

struct MovementContext {
    const MovementIntent&                         intent;
    const World&                                  world;
    bool                                          on_ground;
    bool                                          in_fluid;
    std::function<bool(const std::string& pose)>  fits;
};

class MovementMode {
public:
    MovementMode(std::string id, int priority, std::string pose, double speed_multiplier)
        : m_id(std::move(id)), m_pose(std::move(pose)), m_priority(priority), m_speed(speed_multiplier) {}
    virtual ~MovementMode() = default;

    const std::string& id()               const noexcept { return m_id; }
    const std::string& pose()             const noexcept { return m_pose; }
    int                priority()         const noexcept { return m_priority; }
    double             speed_multiplier() const noexcept { return m_speed; }

    virtual bool   is_active(const MovementContext& ctx) const = 0;
    virtual double acceleration_multiplier() const noexcept { return 1.0; }
    virtual double gravity_scale()           const noexcept { return 1.0; }
    virtual bool   can_jump()                const noexcept { return true; }

    virtual void update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const {
        const vector3d target = wish * (stats.speed * m_speed);
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
    std::string m_id;
    std::string m_pose;
    int         m_priority;
    double      m_speed;
};

class WalkMode final : public MovementMode {
public:
    explicit WalkMode(double speed_multiplier = 1.0) : MovementMode("voxelspire:walk", 0, Poses::Standing, speed_multiplier) {}
    bool is_active(const MovementContext&) const override { return true; }
};

class SprintMode final : public MovementMode {
public:
    explicit SprintMode(double speed_multiplier = 1.3) : MovementMode("voxelspire:sprint", 100, Poses::Standing, speed_multiplier) {}
    bool is_active(const MovementContext& ctx) const override {
        return ctx.intent.sprint && ctx.intent.forward > 0.0 && ctx.fits(pose());
    }
};

class CrouchMode final : public MovementMode {
public:
    explicit CrouchMode(double speed_multiplier = 0.3) : MovementMode("voxelspire:crouch", 200, Poses::Crouching, speed_multiplier) {}
    bool is_active(const MovementContext& ctx) const override {
        return ctx.intent.crouch || !ctx.fits(Poses::Standing);
    }
};

class CrawlMode final : public MovementMode {
public:
    explicit CrawlMode(double speed_multiplier = 0.3) : MovementMode("voxelspire:crawl", 300, Poses::Prone, speed_multiplier) {}
    bool is_active(const MovementContext& ctx) const override {
        return ctx.intent.crawl || !ctx.fits(Poses::Crouching);
    }
    bool can_jump() const noexcept override { return false; }
};

class SwimMode final : public MovementMode {
public:
    explicit SwimMode(double speed_multiplier = 0.45, double rise_speed = 2.0, double sink_speed = 0.5, double vertical_accel = 8.0)
        : MovementMode("voxelspire:swim", 400, Poses::Standing, speed_multiplier),
          m_rise(rise_speed), m_sink(sink_speed), m_vertical_accel(vertical_accel) {}

    bool   is_active(const MovementContext& ctx) const override { return ctx.in_fluid; }
    double acceleration_multiplier() const noexcept override { return 0.5; }
    double gravity_scale()           const noexcept override { return 0.0; }
    bool   can_jump()                const noexcept override { return false; }

    void update_velocity(vector3d& v, const vector3d& wish, const MovementContext& ctx, const MovementStats& stats, double dt) const override {
        const vector3d target = wish * (stats.speed * speed_multiplier());
        const double accel = stats.ground_accel * acceleration_multiplier() * dt;
        v.x = approach(v.x, target.x, accel);
        v.y = approach(v.y, target.y, accel);

        const double vz = ctx.intent.jump ? m_rise : (ctx.intent.crouch ? -m_rise : -m_sink);
        v.z = approach(v.z, vz, m_vertical_accel * dt);
    }

private:
    double m_rise, m_sink, m_vertical_accel;
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

    bool remove(const std::string& id) {
        auto it = std::find_if(m_modes.begin(), m_modes.end(), [&](const auto& m) { return m->id() == id; });
        if (it == m_modes.end()) return false;
        m_modes.erase(it);
        return true;
    }

    const MovementMode* find(const std::string& id) const noexcept {
        for (const auto& m : m_modes) if (m->id() == id) return m.get();
        return nullptr;
    }

    const MovementMode* select(const MovementContext& ctx) const {
        for (const auto& m : m_modes) if (m->is_active(ctx)) return m.get();
        return nullptr;
    }

    std::size_t size() const noexcept { return m_modes.size(); }

    static void register_defaults(MovementModeRegistry& r) {
        r.add<WalkMode>();
        r.add<SprintMode>();
        r.add<CrouchMode>();
        r.add<CrawlMode>();
        r.add<SwimMode>();
    }

private:
    std::vector<std::unique_ptr<MovementMode>> m_modes;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_MOVEMENT_MODE_HPP