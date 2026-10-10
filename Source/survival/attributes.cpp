#include "survival/attributes.hpp"

namespace voxelspire {

void SurvivalAttributes::register_all(AttributeRegistry& r) {
    const SurvivalSettings s;
    r.add(Attributes::MaxHealth,       s.health.max,         0.0, NO_LIMIT);
    r.add(Attributes::MaxHunger,       s.hunger.max,         0.0, NO_LIMIT);
    r.add(Attributes::MaxThirst,       s.thirst.max,         0.0, NO_LIMIT);
    r.add(Attributes::MaxStamina,      s.stamina.max,        0.0, NO_LIMIT);
    r.add(Attributes::MaxBreath,       s.breath.max,         0.0, NO_LIMIT);
    r.add(Attributes::ComfortableLoad, s.weight.comfortable, 0.0, NO_LIMIT);
    r.add(Attributes::CarryLimit,      s.weight.max,         0.0, NO_LIMIT);
}

void SurvivalAttributes::set_bases(AttributeMap& a, const SurvivalSettings& s) {
    base(a, Attributes::MaxHealth,       s.health.max);
    base(a, Attributes::MaxHunger,       s.hunger.max);
    base(a, Attributes::MaxThirst,       s.thirst.max);
    base(a, Attributes::MaxStamina,      s.stamina.max);
    base(a, Attributes::MaxBreath,       s.breath.max);
    base(a, Attributes::ComfortableLoad, s.weight.comfortable);
    base(a, Attributes::CarryLimit,      s.weight.max);
}

SurvivalSettings SurvivalAttributes::effective(const AttributeMap& a, SurvivalSettings s) {
    s.health.max         = a.value(Attributes::MaxHealth);
    s.hunger.max         = a.value(Attributes::MaxHunger);
    s.thirst.max         = a.value(Attributes::MaxThirst);
    s.stamina.max        = a.value(Attributes::MaxStamina);
    s.breath.max         = a.value(Attributes::MaxBreath);
    s.weight.comfortable = a.value(Attributes::ComfortableLoad);
    s.weight.max         = a.value(Attributes::CarryLimit);
    return s;
}

} // namespace voxelspire
