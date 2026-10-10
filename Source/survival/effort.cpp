#include "survival/effort.hpp"

namespace voxelspire {

ActivityCost ExertionTable::cost(const Identifier& activity, const ExertionSettings& s) const {
    for (const Entry& e : m_entries) {
        if (e.activity != activity) continue;
        if (e.fixed) return e.cost;
        if (e.member) return s.*(e.member);
    }

    return s.walk;
}

ExertionTable ExertionTable::defaults() {
    ExertionTable t;
    t.link(Activities::Idle,       &ExertionSettings::idle);
    t.link(MovementModes::Walk,    &ExertionSettings::walk);
    t.link(MovementModes::Sprint,  &ExertionSettings::sprint);
    t.link(MovementModes::Crouch,  &ExertionSettings::crouch);
    t.link(MovementModes::Crawl,   &ExertionSettings::crawl);
    t.link(MovementModes::Swim,    &ExertionSettings::tread);
    t.link(MovementModes::Stroke,  &ExertionSettings::stroke);
    t.link(MovementModes::Flutter, &ExertionSettings::idle);
    t.link(MovementModes::Fly,     &ExertionSettings::fly);
    return t;
}

bool ExertionTable::rests_when_still(const Identifier& activity) noexcept {
    return activity == MovementModes::Walk || activity == MovementModes::Sprint || activity == MovementModes::Crouch || activity == MovementModes::Crawl;
}

auto ExertionTable::entry(const Identifier& activity) -> Entry& {
    for (Entry& e : m_entries) if (e.activity == activity) return e;
    m_entries.push_back({ activity });
    return m_entries.back();
}

bool LoadRegistry::remove(const Identifier& id) {
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
        if (it->id != id) continue;
        m_sources.erase(it);
        return true;
    }

    return false;
}

double LoadRegistry::measure(const Entity& e, std::vector<LoadPart>* parts) const {
    double total = 0.0;
    if (parts) parts->clear();

    for (const Entry& s : m_sources) {
        const double kg = s.source(e);
        if (kg <= 0.0) continue;
        total += kg;
        if (parts) parts->push_back({ s.name, kg });
    }

    return total;
}

} // namespace voxelspire
