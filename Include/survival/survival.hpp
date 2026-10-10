#ifndef VOXELSPIRE_SURVIVAL_SURVIVAL_HPP
#define VOXELSPIRE_SURVIVAL_SURVIVAL_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include "../core/settings.hpp"
#include "../entity/movement_mode.hpp"
#include "../physics/heat.hpp"
#include "damage.hpp"
#include "effort.hpp"

namespace voxelspire {

enum class Vital : std::uint8_t { Health = 0, Hunger, Thirst, Stamina, Breath, Count };

struct Food {
    std::string name;
    double      energy = 0.0;
    double      water  = 0.0;
    double      bulk   = 0.0;
};

struct Stomach {
    double bulk   = 0.0;
    double energy = 0.0;
    double water  = 0.0;

    bool empty() const noexcept { return bulk <= 0.0; }
};

struct SurvivalInput {
    Identifier activity    = Activities::Idle;
    bool       moving      = false;
    bool       jumped      = false;
    double     landed_fall = 0.0;
    bool       head_under  = false;
    bool       pushing     = false;
    BodyState  body        = BodyState::Comfortable;
    double     load        = 0.0;
};

struct SurvivalSnapshot {
    bool    valid   = false;
    bool    dead    = false;
    double  health  = 0.0;
    double  hunger  = 0.0;
    double  thirst  = 0.0;
    double  stamina = 0.0;
    double  breath  = 0.0;
    Stomach stomach;
};

class Survival {
public:
    static constexpr std::size_t VITALS          = static_cast<std::size_t>(Vital::Count);
    static constexpr double      SECONDS_PER_MIN = 60.0;
    static constexpr double      RATE_SMOOTHING  = 2.0;
    static constexpr std::size_t LOG_SIZE        = 8;
    static constexpr int         MAX_HITS        = 64;

    void reset(const SurvivalSettings& s);

    void restore(const SurvivalSnapshot& snap, const SurvivalSettings& s);

    SurvivalSnapshot snapshot() const;

    void fit(const SurvivalSettings& s);

    void update(double dt, const SurvivalSettings& s, const ExertionTable& table, const SurvivalInput& in, double now);

    std::vector<Damage> take_damage() {
        std::vector<Damage> out;
        out.swap(m_pending);
        return out;
    }

    void queue(const Damage& d) { m_pending.push_back(d); }

    double hurt(const Damage& d, const SurvivalSettings& s, const DamageType& type);

    bool eat(const Food& food, const SurvivalSettings& s);

    double drink(double amount, const SurvivalSettings& s);

    bool wants_drink(const SurvivalSettings& s) const noexcept {
        return s.enabled && s.thirst.enabled && !m_dead && value(Vital::Thirst) < s.thirst.max;
    }

    MovementLimits effects(const SurvivalSettings& s, double load_kg) const;

    static double load_level(double kg, const WeightSettings& w) noexcept;

    double value(Vital v) const noexcept { return m_value[index(v)]; }
    double rate(Vital v) const noexcept { return m_rate[index(v)]; }

    void set(Vital v, double amount, const SurvivalSettings& s);

    static double max_of(Vital v, const SurvivalSettings& s) noexcept;

    static bool shown(Vital v, const SurvivalSettings& s) noexcept;

    double fraction(Vital v, const SurvivalSettings& s) const noexcept {
        const double m = max_of(v, s);
        return m > 0.0 ? vclamp(value(v) / m, 0.0, 1.0) : 0.0;
    }

    bool stuffed(const SurvivalSettings& s) const noexcept;

    bool dead()       const noexcept { return m_dead; }
    bool exhausted()  const noexcept { return m_exhausted; }
    bool starving()   const noexcept { return m_starving > 0.0; }
    bool parched()    const noexcept { return m_parched > 0.0; }
    bool drowning(const SurvivalSettings& s) const noexcept { return s.breath.enabled && value(Vital::Breath) <= 0.0; }
    bool regenerating() const noexcept { return m_regenerating; }

    static bool weak(const NeedSettings& n, double v) noexcept { return n.enabled && v < n.weak_below * n.max; }

    const Stomach& stomach() const noexcept { return m_stomach; }
    double digest_seconds(const SurvivalSettings& s) const noexcept { return s.digestion.rate > 0.0 ? m_stomach.bulk / s.digestion.rate * SECONDS_PER_MIN : 0.0; }

    const Identifier&               cause()  const noexcept { return m_cause; }
    const std::deque<DamageRecord>& recent() const noexcept { return m_log; }

private:
    enum Tick : std::uint8_t { StarveTick = 0, ThirstTick, DrownTick, ExhaustTick, FreezeTick, HeatTick, TickCount };

    struct Ticker {
        double time = 0.0;

        int step(double dt, double interval) noexcept;
    };

    static constexpr std::size_t index(Vital v) noexcept { return static_cast<std::size_t>(v); }
    static double start_of(double start, double max) noexcept { return vclamp(start, 0.0, max); }

    void clamp_to(Vital v, double max) noexcept { m_value[index(v)] = vclamp(m_value[index(v)], 0.0, max); }

    void fill(const SurvivalSettings& s) noexcept;

    double add(Vital v, double amount, double max, bool enabled) noexcept;

    void take(Vital v, double amount) noexcept {
        double& x = m_value[index(v)];
        x = vmax(x - amount, 0.0);
    }

    static void weaken(MovementLimits& e, const NeedSettings& n, double v) noexcept {
        if (!weak(n, v)) return;
        e.speed *= n.weak_speed;
        if (n.weak_no_sprint) e.can_sprint = false;
    }

    double stamina(double dt, const SurvivalSettings& s, const ActivityCost& cost, double load, const SurvivalInput& in);

    void needs(double dt, const SurvivalSettings& s, const ActivityCost& cost, double load, double spent, const SurvivalInput& in);

    void digest(double dt, const SurvivalSettings& s);

    void regenerate(double dt, const SurvivalSettings& s);

    void periodic(Tick tick, double dt, double interval, Identifier type, double amount, double floor);

    void idle(Tick tick) noexcept { m_tickers[tick] = {}; }

    void lack(Tick tick, double dt, const NeedSettings& n, double v, double& time, Identifier type);

    void harm(double dt, const SurvivalSettings& s, const SurvivalInput& in);

    void track(const std::array<double, VITALS>& before, double dt) noexcept;

    void log(const Identifier& type, double amount);

    static constexpr double MERGE_SECONDS = 1.5;

    std::array<double, VITALS>    m_value{};
    std::array<double, VITALS>    m_rate{};
    std::array<Ticker, TickCount> m_tickers{};
    Stomach                       m_stomach;
    std::vector<Damage>           m_pending;
    std::deque<DamageRecord>      m_log;
    Identifier                    m_cause;
    bool                          m_exhausted    = false;
    bool                          m_dead         = false;
    bool                          m_regenerating = false;
    double                        m_rested       = 0.0;
    double                        m_hurt_ago     = 0.0;
    double                        m_starving     = 0.0;
    double                        m_parched      = 0.0;
    double                        m_now          = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SURVIVAL_SURVIVAL_HPP