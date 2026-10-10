#include "survival/player_survival.hpp"

namespace voxelspire {

PlayerSurvival::PlayerSurvival(const GameSettings& settings) : m_settings(&settings), m_effective(settings.survival), m_table(ExertionTable::defaults()) {
    DamageTypeRegistry::register_defaults(m_types);
    m_loads.add(LoadSources::Carried, "Carried", [this](const Entity&) { return m_settings->survival.weight.carried; });
}

void PlayerSurvival::refresh(Player& p) {
    SurvivalAttributes::set_bases(p.attributes(), m_settings->survival);
    m_effective = SurvivalAttributes::effective(p.attributes(), m_settings->survival);
}

void PlayerSurvival::tick(double dt, Player& p, World& world, const SurvivalFrame& frame) {
    refresh(p);
    const SurvivalSettings& s = settings();
    m_vitals.fit(s);
    const MovementReport& r = p.report();
    m_load = m_loads.measure(p, &m_load_parts);

    SurvivalInput in;
    in.activity    = r.activity ? r.activity : Activities::Idle;
    in.moving      = r.moving;
    in.jumped      = r.jumped;
    in.landed_fall = r.landed_fall;
    in.head_under  = r.head_under;
    in.pushing     = r.straining;
    in.body        = frame.body;
    in.load        = m_load;
    m_vitals.update(dt, s, m_table, in, frame.now);
    for (const Damage& d : m_vitals.take_damage()) hurt(d, p);
    drink(dt, p, world, frame);
    apply_limits(p);
}

double PlayerSurvival::hurt(const Damage& raw, const Player& p) {
    const Damage d = m_pipeline.run(raw, p);
    return m_vitals.hurt(d, settings(), m_types.get(d.type));
}

bool PlayerSurvival::fell_out(const Player& p) {
    const SurvivalSettings& s = settings();
    if (!s.enabled || !s.health.enabled || !s.health.void_kills) return false;
    hurt(Damage::of(DamageTypes::Void, HUGE_DAMAGE), p);
    return true;
}

std::string PlayerSurvival::death_message() const {
    const DamageType* t = m_types.find(m_vitals.cause());
    return t ? t->death_message : std::string("You died");
}

DrinkTarget PlayerSurvival::find_water(const World& world, const vector3d& eye, const vector3d& dir, double reach, BlockId water) {
    DrinkTarget out;
    const fizmo::geometry::Ray3D ray{ eye, dir };

    fizmo::geometry::raycast_grid(ray, reach, [&](int x, int y, int z) {
        const BlockPos c{ x, y, z };
        if (world.block_id_at(c) == water && world.fluid_height(c) > 0.0) { out.cell = c; out.found = true; return true; }
        return world.is_solid(c);
    });

    return out;
}

void PlayerSurvival::drink(double dt, const Player& p, World& world, const SurvivalFrame& frame) {
    const DrinkSettings& d = settings().drink;
    m_target = {};

    if (!frame.drinking || !d.from_water || !m_vitals.wants_drink(settings())) {
        m_sip = 0.0;
        return;
    }

    m_target = find_water(world, p.eye_position(), p.look_direction(), p.attributes().value(Attributes::BlockReach), frame.water);
    if (!m_target.found) { m_sip = 0.0; return; }
    m_sip += dt;
    if (m_sip < d.seconds) return;
    m_sip = 0.0;
    m_vitals.drink(d.amount, settings());
    if (d.takes_water && frame.realistic_water) sip_from(world, m_target.cell, frame.water);
}

void PlayerSurvival::sip_from(World& world, const BlockPos& c, BlockId water) {
    const int have = RealisticFluid::units(world.fluid_state(c));
    if (have <= 1) world.set_block(c, AIR_ID);
    else world.set_block(c, water, RealisticFluid::state_for(have - 1));
}

void PlayerSurvival::apply_limits(Player& p) const {
    MovementLimits l = m_vitals.effects(settings(), m_load);

    if (m_vitals.dead()) {
        l.can_sprint = false;
        l.can_jump   = false;
        l.speed      = 0.0;
    }

    p.set_limits(l);
}

} // namespace voxelspire
