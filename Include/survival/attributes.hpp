#ifndef VOXELSPIRE_SURVIVAL_ATTRIBUTES_HPP
#define VOXELSPIRE_SURVIVAL_ATTRIBUTES_HPP

#include "../core/settings.hpp"
#include "../entity/attributes.hpp"

namespace voxelspire {

namespace Attributes {
    inline const Identifier MaxHealth       = core_id(Kind::Attribute, "max_health");
    inline const Identifier MaxHunger       = core_id(Kind::Attribute, "max_hunger");
    inline const Identifier MaxThirst       = core_id(Kind::Attribute, "max_thirst");
    inline const Identifier MaxStamina      = core_id(Kind::Attribute, "max_stamina");
    inline const Identifier MaxBreath       = core_id(Kind::Attribute, "max_breath");
    inline const Identifier ComfortableLoad = core_id(Kind::Attribute, "comfortable_load");
    inline const Identifier CarryLimit      = core_id(Kind::Attribute, "carry_limit");
} // namespace Attributes

struct SurvivalAttributes {
    static constexpr double NO_LIMIT = AttributeRegistry::NO_LIMIT;

    static void register_all(AttributeRegistry& r);

    static void set_bases(AttributeMap& a, const SurvivalSettings& s);

    static SurvivalSettings effective(const AttributeMap& a, SurvivalSettings s);

private:
    static void base(AttributeMap& a, const Identifier& id, double v) {
        AttributeInstance& i = a.get(id);
        if (i.base() != v) i.set_base(v);
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_SURVIVAL_ATTRIBUTES_HPP