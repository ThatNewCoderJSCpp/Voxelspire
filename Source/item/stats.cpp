#include "item/stats.hpp"

namespace voxelspire {

bool ItemStatRegistry::remove(const Identifier& id) {
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
        if (it->first != id) continue;
        m_sources.erase(it);
        return true;
    }

    return false;
}

std::string ItemStatRegistry::recipe_text(const Recipe& r, const ItemRegistry& items) {
    std::string out = r.station + ": ";
    bool first = true;

    for (const RecipeIngredient& in : r.inputs) {
        if (in.item == NO_ITEM) continue;
        const Item* i = items.get(in.item);
        if (!first) out += ", ";
        out += std::to_string(in.count) + " " + (i ? i->name() : std::string("?"));
        first = false;
    }

    const Item* o = items.get(r.output.item);
    return out + " makes " + std::to_string(r.output.count) + " " + (o ? o->name() : std::string("?"));
}

void ItemStatRegistry::register_defaults(ItemStatRegistry& r) {
    r.add(core_id(Kind::Item, { "stat", "basics" }), [](const Item& item, const ItemStatContext& c, std::vector<ItemStat>& out) {
        out.push_back({ "ID", item.id().str() });
        out.push_back({ "Weight", number(item.properties().weight, " kg") });
        if (c.rules) out.push_back({ "Stack size", std::to_string(c.rules->limit(item.handle())) + (c.rules->overridden(item.handle()) ? " (changed in this world)" : "") });
        if (!item.properties().description.empty()) out.push_back({ "About", item.properties().description });
    });

    r.add(core_id(Kind::Item, { "stat", "block" }), [](const Item& item, const ItemStatContext& c, std::vector<ItemStat>& out) {
        if (!item.places_block() || !c.blocks) return;
        const BlockTraits& t = c.blocks->traits(item.properties().block);
        out.push_back({ "Places", c.blocks->get(item.properties().block).identifier().str() });
        out.push_back({ "Solid", t.solid ? "yes" : "no" });
        out.push_back({ "See-through", t.opaque ? "no" : "yes" });
        if (t.emission.emits()) out.push_back({ "Light", std::to_string(t.emission.level()) });
        if (t.thermal.heat != 0.0) out.push_back({ "Heat", number(t.thermal.heat, " C") });
        out.push_back({ "Holds back heat", number(t.thermal.insulation) });
        out.push_back({ "Conducts heat", number(t.thermal.conductivity, "x") });
        if (t.thermal.max_temperature < BlockThermal::NO_LIMIT) out.push_back({ "Never warmer than", number(t.thermal.max_temperature, " C") });
        if (t.thermal.min_temperature > -BlockThermal::NO_LIMIT) out.push_back({ "Never colder than", number(t.thermal.min_temperature, " C") });
    });
}

} // namespace voxelspire
