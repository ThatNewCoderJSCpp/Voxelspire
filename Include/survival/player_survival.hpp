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

    explicit PlayerSurvival(const GameSettings& settings);

    DamageTypeRegistry&       damage_types()       noexcept { return m_types; }
    const DamageTypeRegistry& damage_types() const noexcept { return m_types; }
    DamagePipeline&           pipeline()           noexcept { return m_pipeline; }
    ExertionTable&            exertion()           noexcept { return m_table; }
    LoadRegistry&             loads()              noexcept { return m_loads; }
    const Survival&           vitals()       const noexcept { return m_vitals; }
    Survival&                 vitals()             noexcept { return m_vitals; }

    const SurvivalSettings& settings() const noexcept { return m_effective; }

    void refresh(Player& p);

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

    void tick(double dt, Player& p, World& world, const SurvivalFrame& frame);

    double hurt(const Damage& raw, const Player& p);

    bool fell_out(const Player& p);

    bool eat(const Food& food) { return m_vitals.eat(food, settings()); }

    ActivityCost exertion_cost(const Identifier& activity, const ExertionSettings& e) const { return m_table.cost(activity, e); }

    bool dead() const noexcept { return m_vitals.dead(); }

    std::string death_message() const;

    double                       load()       const noexcept { return m_load; }
    const std::vector<LoadPart>& load_parts() const noexcept { return m_load_parts; }
    const DrinkTarget&           drink_target() const noexcept { return m_target; }

    static DrinkTarget find_water(const World& world, const vector3d& eye, const vector3d& dir, double reach, BlockId water);

private:
    void drink(double dt, const Player& p, World& world, const SurvivalFrame& frame);

    static void sip_from(World& world, const BlockPos& c, BlockId water);

    void apply_limits(Player& p) const;

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