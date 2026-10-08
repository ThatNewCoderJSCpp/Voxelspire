#ifndef VOXELSPIRE_SURVIVAL_PLAYER_SURVIVAL_HPP
#define VOXELSPIRE_SURVIVAL_PLAYER_SURVIVAL_HPP

#include <optional>
#include <string>
#include <vector>
#include "../entity/player.hpp"
#include "../physics/fluid_flow.hpp"
#include "../world/world.hpp"
#include "attributes.hpp"
#include "survival.hpp"

namespace voxelspire {

namespace LoadSources {
    inline const Identifier Carried  = core_id(Kind::Survival, "carried");
} // namespace LoadSources

struct DrinkTarget {
    BlockPos cell{};
    bool     found = false;
};

struct SurvivalFrame {
    BodyState body            = BodyState::Comfortable;
    bool      drinking        = false;
    bool      realistic_water = false;
    BlockId   water           = AIR_ID;
    double    now             = 0.0;
};

class PlayerSurvival {
public:
    static constexpr double HUGE_DAMAGE = 1.0e9;

    explicit PlayerSurvival(const GameSettings& settings) : m_settings(&settings), m_effective(settings.survival), m_table(ExertionTable::defaults()) {
        DamageTypeRegistry::register_defaults(m_types);
        m_loads.add(LoadSources::Carried, "Carried", [this](const Entity&) { return m_settings->survival.weight.carried; });
    }

    DamageTypeRegistry&       damage_types()       noexcept { return m_types; }
    const DamageTypeRegistry& damage_types() const noexcept { return m_types; }
    DamagePipeline&           pipeline()           noexcept { return m_pipeline; }
    ExertionTable&            exertion()           noexcept { return m_table; }
    LoadRegistry&             loads()              noexcept { return m_loads; }
    const Survival&           vitals()       const noexcept { return m_vitals; }
    Survival&                 vitals()             noexcept { return m_vitals; }

    const SurvivalSettings& settings() const noexcept { return m_effective; }

    void refresh(Player& p) {
        SurvivalAttributes::set_bases(p.attributes(), m_settings->survival);
        m_effective = SurvivalAttributes::effective(p.attributes(), m_settings->survival);
    }

    void reset(Player& p) {
        refresh(p);
        m_vitals.reset(settings());
        m_sip = 0.0;
        p.set_limits({});
    }

    void restore(const SurvivalSnapshot& snap, Player& p) {
        refresh(p);
        m_vitals.restore(snap, settings());
        m_sip = 0.0;
        apply_limits(p);
    }

    SurvivalSnapshot snapshot() const { return m_vitals.snapshot(); }

    void tick(double dt, Player& p, World& world, const SurvivalFrame& frame) {
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

    double hurt(const Damage& raw, const Player& p) {
        const Damage d = m_pipeline.run(raw, p);
        return m_vitals.hurt(d, settings(), m_types.get(d.type));
    }

    bool fell_out(const Player& p) {
        const SurvivalSettings& s = settings();
        if (!s.enabled || !s.health.enabled || !s.health.void_kills) return false;
        hurt(Damage::of(DamageTypes::Void, HUGE_DAMAGE), p);
        return true;
    }

    bool eat(const Food& food) { return m_vitals.eat(food, settings()); }

    ActivityCost exertion_cost(const Identifier& activity, const ExertionSettings& e) const { return m_table.cost(activity, e); }

    bool dead() const noexcept { return m_vitals.dead(); }

    std::string death_message() const {
        const DamageType* t = m_types.find(m_vitals.cause());
        return t ? t->death_message : std::string("You died");
    }

    double                       load()       const noexcept { return m_load; }
    const std::vector<LoadPart>& load_parts() const noexcept { return m_load_parts; }
    const DrinkTarget&           drink_target() const noexcept { return m_target; }

    static DrinkTarget find_water(const World& world, const vector3d& eye, const vector3d& dir, double reach, BlockId water) {
        DrinkTarget out;
        const fizmo::geometry::Ray3D ray{ eye, dir };

        fizmo::geometry::raycast_grid(ray, reach, [&](int x, int y, int z) {
            const BlockPos c{ x, y, z };
            if (world.block_id_at(c) == water && world.fluid_height(c) > 0.0) { out.cell = c; out.found = true; return true; }
            return world.is_solid(c);
        });

        return out;
    }

private:
    void drink(double dt, const Player& p, World& world, const SurvivalFrame& frame) {
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

    static void sip_from(World& world, const BlockPos& c, BlockId water) {
        const int have = RealisticFluid::units(world.fluid_state(c));
        if (have <= 1) world.set_block(c, AIR_ID);
        else world.set_block(c, water, RealisticFluid::state_for(have - 1));
    }

    void apply_limits(Player& p) const {
        MovementLimits l = m_vitals.effects(settings(), m_load);

        if (m_vitals.dead()) {
            l.can_sprint = false;
            l.can_jump   = false;
            l.speed      = 0.0;
        }

        p.set_limits(l);
    }

    const GameSettings*   m_settings;
    SurvivalSettings      m_effective;
    Survival              m_vitals;
    DamageTypeRegistry    m_types;
    DamagePipeline        m_pipeline;
    ExertionTable         m_table;
    LoadRegistry          m_loads;
    std::vector<LoadPart> m_load_parts;
    DrinkTarget           m_target;
    double                m_load = 0.0;
    double                m_sip  = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SURVIVAL_PLAYER_SURVIVAL_HPP