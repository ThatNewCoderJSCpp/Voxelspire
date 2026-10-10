#include "survival/survival.hpp"

namespace voxelspire {

void Survival::reset(const SurvivalSettings& s) {
    m_value[index(Vital::Health)]  = start_of(s.health.start, s.health.max);
    m_value[index(Vital::Hunger)]  = start_of(s.hunger.start, s.hunger.max);
    m_value[index(Vital::Thirst)]  = start_of(s.thirst.start, s.thirst.max);
    m_value[index(Vital::Stamina)] = start_of(s.stamina.start, s.stamina.max);
    m_value[index(Vital::Breath)]  = s.breath.max;
    m_rate.fill(0.0);
    m_stomach   = {};
    m_exhausted = false;
    m_dead      = false;
    m_rested    = 0.0;
    m_hurt_ago  = s.health.regen_delay;
    m_starving  = 0.0;
    m_parched   = 0.0;
    m_cause     = Identifier();
    for (Ticker& t : m_tickers) t = {};
    m_pending.clear();
}

void Survival::restore(const SurvivalSnapshot& snap, const SurvivalSettings& s) {
    reset(s);
    if (!snap.valid) return;
    m_value[index(Vital::Health)]  = vclamp(snap.health, 0.0, s.health.max);
    m_value[index(Vital::Hunger)]  = vclamp(snap.hunger, 0.0, s.hunger.max);
    m_value[index(Vital::Thirst)]  = vclamp(snap.thirst, 0.0, s.thirst.max);
    m_value[index(Vital::Stamina)] = vclamp(snap.stamina, 0.0, s.stamina.max);
    m_value[index(Vital::Breath)]  = vclamp(snap.breath, 0.0, s.breath.max);
    m_stomach = snap.stomach;
    m_dead    = snap.dead && m_value[index(Vital::Health)] <= 0.0;
}

SurvivalSnapshot Survival::snapshot() const {
    SurvivalSnapshot snap;
    snap.valid   = true;
    snap.dead    = m_dead;
    snap.health  = value(Vital::Health);
    snap.hunger  = value(Vital::Hunger);
    snap.thirst  = value(Vital::Thirst);
    snap.stamina = value(Vital::Stamina);
    snap.breath  = value(Vital::Breath);
    snap.stomach = m_stomach;
    return snap;
}

void Survival::fit(const SurvivalSettings& s) {
    clamp_to(Vital::Health, s.health.max);
    clamp_to(Vital::Hunger, s.hunger.max);
    clamp_to(Vital::Thirst, s.thirst.max);
    clamp_to(Vital::Stamina, s.stamina.max);
    clamp_to(Vital::Breath, s.breath.max);
}

void Survival::update(double dt, const SurvivalSettings& s, const ExertionTable& table, const SurvivalInput& in, double now) {
    if (dt <= 0.0) return;
    std::array<double, VITALS> before = m_value;
    m_now = now;

    if (!s.enabled || m_dead) {
        if (!s.enabled) fill(s);
        track(before, dt);
        return;
    }

    if (!s.health.enabled) m_value[index(Vital::Health)] = s.health.max;
    if (!s.hunger.enabled) m_value[index(Vital::Hunger)] = s.hunger.max;
    if (!s.thirst.enabled) m_value[index(Vital::Thirst)] = s.thirst.max;
    if (!s.stamina.enabled) { m_value[index(Vital::Stamina)] = s.stamina.max; m_exhausted = false; }
    if (!s.breath.enabled) m_value[index(Vital::Breath)] = s.breath.max;

    const Identifier activity = in.moving || !ExertionTable::rests_when_still(in.activity) ? in.activity : Activities::Idle;
    const ActivityCost cost = table.cost(activity, s.effort);
    const double load = s.weight.enabled ? load_level(in.load, s.weight) : 0.0;
    const double spent = stamina(dt, s, cost, load, in);
    needs(dt, s, cost, load, spent, in);
    digest(dt, s);
    regenerate(dt, s);
    harm(dt, s, in);
    m_hurt_ago += dt;
    track(before, dt);
}

double Survival::hurt(const Damage& d, const SurvivalSettings& s, const DamageType& type) {
    if (!s.enabled || !s.health.enabled || m_dead || d.amount <= 0.0) return 0.0;
    double& hp = m_value[index(Vital::Health)];
    const double amount = type.ignores_scale ? d.amount : d.amount * s.health.damage_scale;
    if (amount <= 0.0) return 0.0;
    double floor = d.floor;
    if (!s.health.can_die && !type.ignores_scale) floor = vmax(floor, vmin(s.health.lowest, s.health.max));
    const double next = floor > 0.0 ? vmax(hp - amount, vmin(floor, hp)) : hp - amount;
    const double dealt = hp - next;
    hp = vmax(next, 0.0);
    m_hurt_ago = 0.0;
    if (dealt > 0.0) log(d.type, dealt);

    if (hp <= 0.0) {
        m_dead  = true;
        m_cause = d.type;
    }

    return dealt;
}

bool Survival::eat(const Food& food, const SurvivalSettings& s) {
    if (!s.enabled || m_dead) return false;

    if (!s.digestion.enabled) {
        add(Vital::Hunger, food.energy, s.hunger.max, s.hunger.enabled);
        add(Vital::Thirst, food.water, s.thirst.max, s.thirst.enabled);
        return true;
    }

    if (m_stomach.bulk + food.bulk > s.digestion.capacity && !m_stomach.empty()) return false;
    m_stomach.bulk   += food.bulk;
    m_stomach.energy += food.energy;
    m_stomach.water  += food.water;
    return true;
}

double Survival::drink(double amount, const SurvivalSettings& s) {
    if (!s.enabled || !s.thirst.enabled || m_dead) return 0.0;
    return add(Vital::Thirst, amount, s.thirst.max, true);
}

MovementLimits Survival::effects(const SurvivalSettings& s, double load_kg) const {
    MovementLimits e;
    if (!s.enabled || m_dead) return e;
    weaken(e, s.hunger, value(Vital::Hunger));
    weaken(e, s.thirst, value(Vital::Thirst));
    if (stuffed(s)) e.speed *= s.digestion.stuffed_speed;

    if (s.stamina.enabled && m_exhausted) {
        if (s.stamina.sprint == ExhaustedSprint::Blocked) e.can_sprint = false;
        if (s.stamina.sprint == ExhaustedSprint::Slower) e.sprint_speed *= s.stamina.sprint_speed;
        if (s.stamina.no_jump) e.can_jump = false;
        e.sink  += s.stamina.sink;
        e.rise *= s.stamina.rise;
    }

    if (s.weight.enabled) {
        const double load = load_level(load_kg, s.weight);

        if (load > 1.0) {
            e.speed *= s.weight.overloaded_speed;
            if (s.weight.overloaded_no_jump) e.can_jump = false;
            if (s.weight.overloaded_no_sprint) e.can_sprint = false;
        } else if (load > 0.0) {
            e.speed *= 1.0 + (s.weight.heavy_speed - 1.0) * load;
        }

        e.sink += s.weight.swim_sink * vmin(load, 1.0);
    }

    return e;
}

double Survival::load_level(double kg, const WeightSettings& w) noexcept {
    if (kg <= w.comfortable) return 0.0;
    const double span = w.max - w.comfortable;
    if (span <= 0.0) return kg > w.max ? 1.0 + (kg - w.max) : 1.0;
    return (kg - w.comfortable) / span;
}

void Survival::set(Vital v, double amount, const SurvivalSettings& s) {
    m_value[index(v)] = vclamp(amount, 0.0, max_of(v, s));
    if (v == Vital::Health && m_value[index(v)] > 0.0) m_dead = false;
}

double Survival::max_of(Vital v, const SurvivalSettings& s) noexcept {
    switch (v) {
        case Vital::Health:  return s.health.max;
        case Vital::Hunger:  return s.hunger.max;
        case Vital::Thirst:  return s.thirst.max;
        case Vital::Stamina: return s.stamina.max;
        case Vital::Breath:  return s.breath.max;
        case Vital::Count:   break;
    }

    return 1.0;
}

bool Survival::shown(Vital v, const SurvivalSettings& s) noexcept {
    if (!s.enabled) return false;

    switch (v) {
        case Vital::Health:  return s.health.enabled;
        case Vital::Hunger:  return s.hunger.enabled;
        case Vital::Thirst:  return s.thirst.enabled;
        case Vital::Stamina: return s.stamina.enabled;
        case Vital::Breath:  return s.breath.enabled;
        case Vital::Count:   break;
    }

    return false;
}

bool Survival::stuffed(const SurvivalSettings& s) const noexcept {
    return s.digestion.enabled && s.digestion.capacity > 0.0 && m_stomach.bulk >= s.digestion.stuffed_at * s.digestion.capacity;
}

int Survival::Ticker::step(double dt, double interval) noexcept {
    if (interval <= 0.0) return 0;
    time += dt;
    int hits = 0;

    while (time >= interval && hits < MAX_HITS) {
        time -= interval;
        ++hits;
    }

    return hits;
}

void Survival::fill(const SurvivalSettings& s) noexcept {
    for (std::size_t i = 0; i < VITALS; ++i) m_value[i] = max_of(static_cast<Vital>(i), s);
    m_exhausted = false;
}

double Survival::add(Vital v, double amount, double max, bool enabled) noexcept {
    if (!enabled || amount <= 0.0) return 0.0;
    double& x = m_value[index(v)];
    const double before = x;
    x = vmin(x + amount, max);
    return x - before;
}

double Survival::stamina(double dt, const SurvivalSettings& s, const ActivityCost& cost, double load, const SurvivalInput& in) {
    const StaminaSettings& st = s.stamina;
    if (!st.enabled) return 0.0;
    const double heavy = 1.0 + s.weight.stamina_extra * vmin(load, 1.0);
    double use = cost.stamina * heavy * dt;
    if (in.jumped) use += s.effort.jump_stamina * heavy;
    const double have = value(Vital::Stamina);
    const double spent = vmin(use, have);
    take(Vital::Stamina, use);
    m_rested = use > 0.0 ? 0.0 : m_rested + dt;

    if (m_rested >= st.regen_delay) {
        const bool hungry = weak(s.hunger, value(Vital::Hunger)) || weak(s.thirst, value(Vital::Thirst));
        add(Vital::Stamina, st.regen * (hungry ? st.weak_regen : 1.0) * dt, st.max, true);
    }

    const double now = value(Vital::Stamina);
    if (now <= 0.0) m_exhausted = true;
    else if (m_exhausted && now >= st.recover_at * st.max) m_exhausted = false;
    return spent;
}

void Survival::needs(double dt, const SurvivalSettings& s, const ActivityCost& cost, double load, double spent, const SurvivalInput& in) {
    const double minutes = dt / SECONDS_PER_MIN;
    const double heavy   = 1.0 + s.weight.hunger_extra * vmin(load, 1.0);
    const double stamina_share = s.stamina.max > 0.0 ? spent / s.stamina.max : 0.0;

    if (s.hunger.enabled) {
        const double cold = in.body == BodyState::Cold || in.body == BodyState::Freezing ? s.climate.cold_hunger : 1.0;
        double use = s.hunger.drain * cost.hunger * heavy * cold * minutes;
        use += stamina_share * s.stamina.hunger_cost;
        if (in.jumped) use += s.effort.jump_hunger;
        take(Vital::Hunger, use);
    }

    if (s.thirst.enabled) {
        double hot = 1.0;
        if (in.body == BodyState::Hot) hot = s.climate.hot_thirst;
        if (in.body == BodyState::Overheating) hot = s.climate.overheating_thirst;
        double use = s.thirst.drain * cost.thirst * heavy * hot * minutes;
        use += stamina_share * s.stamina.thirst_cost;
        if (in.jumped) use += s.effort.jump_thirst;
        take(Vital::Thirst, use);
    }
}

void Survival::digest(double dt, const SurvivalSettings& s) {
    if (m_stomach.empty()) return;

    if (!s.digestion.enabled) {
        add(Vital::Hunger, m_stomach.energy, s.hunger.max, s.hunger.enabled);
        add(Vital::Thirst, m_stomach.water, s.thirst.max, s.thirst.enabled);
        m_stomach = {};
        return;
    }

    const double amount = vmin(m_stomach.bulk, s.digestion.rate * dt / SECONDS_PER_MIN);
    const double share  = amount / m_stomach.bulk;
    const double energy = m_stomach.energy * share, water = m_stomach.water * share;
    m_stomach.bulk   -= amount;
    m_stomach.energy -= energy;
    m_stomach.water  -= water;
    add(Vital::Hunger, energy, s.hunger.max, s.hunger.enabled);
    add(Vital::Thirst, water, s.thirst.max, s.thirst.enabled);
    if (m_stomach.bulk <= 0.0) m_stomach = {};
}

void Survival::regenerate(double dt, const SurvivalSettings& s) {
    const HealthSettings& h = s.health;
    m_regenerating = false;
    if (!h.enabled || !h.regenerates || m_hurt_ago < h.regen_delay) return;
    if (value(Vital::Health) >= h.max) return;
    if (s.hunger.enabled && value(Vital::Hunger) < h.regen_hunger * s.hunger.max) return;
    if (s.thirst.enabled && value(Vital::Thirst) < h.regen_thirst * s.thirst.max) return;
    const double healed = add(Vital::Health, h.regen_rate * dt / SECONDS_PER_MIN, h.max, true);
    if (s.hunger.enabled) take(Vital::Hunger, healed * h.regen_cost);
    m_regenerating = healed > 0.0;
}

void Survival::periodic(Tick tick, double dt, double interval, Identifier type, double amount, double floor) {
    const int hits = m_tickers[tick].step(dt, interval);
    for (int i = 0; i < hits; ++i) queue(Damage::of(type, amount, floor));
}

void Survival::lack(Tick tick, double dt, const NeedSettings& n, double v, double& time, Identifier type) {
    if (!n.enabled || v > 0.0) {
        time = 0.0;
        idle(tick);
        return;
    }

    time += dt;
    if (!n.hurts) return;
    const double speed = n.style == LossStyle::Building ? 1.0 + n.building * time / SECONDS_PER_MIN : 1.0;
    periodic(tick, dt, n.interval / speed, type, n.damage, n.can_kill ? 0.0 : n.lowest);
}

void Survival::harm(double dt, const SurvivalSettings& s, const SurvivalInput& in) {
    lack(StarveTick, dt, s.hunger, value(Vital::Hunger), m_starving, DamageTypes::Starving);
    lack(ThirstTick, dt, s.thirst, value(Vital::Thirst), m_parched, DamageTypes::Dehydration);

    if (s.breath.enabled) {
        if (in.head_under) take(Vital::Breath, dt);
        else add(Vital::Breath, s.breath.recover * dt, s.breath.max, true);
        if (value(Vital::Breath) <= 0.0) periodic(DrownTick, dt, s.breath.interval, DamageTypes::Drowning, s.breath.damage, 0.0);
        else idle(DrownTick);
    }

    if (s.stamina.enabled && s.stamina.hurts && m_exhausted && in.pushing) periodic(ExhaustTick, dt, s.stamina.interval, DamageTypes::Exhaustion, s.stamina.damage, 0.0);
    else idle(ExhaustTick);

    const ClimateHarmSettings& c = s.climate;
    if (c.freezing_hurts && in.body == BodyState::Freezing) periodic(FreezeTick, dt, c.freezing_interval, DamageTypes::Freezing, c.freezing_damage, 0.0);
    else idle(FreezeTick);
    if (c.overheating_hurts && in.body == BodyState::Overheating) periodic(HeatTick, dt, c.overheating_interval, DamageTypes::Overheating, c.overheating_damage, 0.0);
    else idle(HeatTick);

    if (s.health.fall_damage && in.landed_fall > s.health.safe_fall) {
        const double amount = (in.landed_fall - s.health.safe_fall) * s.health.fall_per_block;
        if (amount > 0.0) queue(Damage::of(DamageTypes::Fall, amount));
    }
}

void Survival::track(const std::array<double, VITALS>& before, double dt) noexcept {
    const double blend = vmin(1.0, dt / RATE_SMOOTHING);

    for (std::size_t i = 0; i < VITALS; ++i) {
        const double per_min = (m_value[i] - before[i]) / dt * SECONDS_PER_MIN;
        m_rate[i] += (per_min - m_rate[i]) * blend;
    }
}

void Survival::log(const Identifier& type, double amount) {
    if (!m_log.empty() && m_log.front().type == type && m_now - m_log.front().time < MERGE_SECONDS) {
        m_log.front().amount += amount;
        m_log.front().time = m_now;
        return;
    }

    m_log.push_front({ type, amount, m_now });
    while (m_log.size() > LOG_SIZE) m_log.pop_back();
}

} // namespace voxelspire
