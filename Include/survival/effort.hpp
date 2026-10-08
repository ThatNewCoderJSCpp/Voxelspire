#ifndef VOXELSPIRE_SURVIVAL_EFFORT_HPP
#define VOXELSPIRE_SURVIVAL_EFFORT_HPP

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "../core/identifier.hpp"
#include "../entity/entity.hpp"
#include "../entity/movement_mode.hpp"
#include "../core/settings.hpp"

namespace voxelspire {

namespace Activities {
    inline const Identifier Idle = core_id(Kind::Survival, "idle");
} // namespace Activities

class ExertionTable {
public:
    using Member = ActivityCost ExertionSettings::*;

    void link(const Identifier& activity, Member member) {
        Entry& e = entry(activity);
        e.member = member;
        e.fixed  = false;
    }

    void set(const Identifier& activity, const ActivityCost& cost) {
        Entry& e = entry(activity);
        e.cost  = cost;
        e.fixed = true;
    }

    ActivityCost cost(const Identifier& activity, const ExertionSettings& s) const {
        for (const Entry& e : m_entries) {
            if (e.activity != activity) continue;
            if (e.fixed) return e.cost;
            if (e.member) return s.*(e.member);
        }

        return s.walk;
    }

    static ExertionTable defaults() {
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

    static bool rests_when_still(const Identifier& activity) noexcept {
        return activity == MovementModes::Walk || activity == MovementModes::Sprint || activity == MovementModes::Crouch || activity == MovementModes::Crawl;
    }

private:
    struct Entry {
        Identifier   activity;
        Member       member = nullptr;
        ActivityCost cost;
        bool         fixed  = false;
    };

    Entry& entry(const Identifier& activity) {
        for (Entry& e : m_entries) if (e.activity == activity) return e;
        m_entries.push_back({ activity });
        return m_entries.back();
    }

    std::vector<Entry> m_entries;
};

struct LoadPart {
    std::string name;
    double      kilograms = 0.0;
};

class LoadRegistry {
public:
    using Source = std::function<double(const Entity&)>;

    void add(Identifier id, std::string name, Source source) {
        remove(id);
        m_sources.push_back({ std::move(id), std::move(name), std::move(source) });
    }

    bool remove(const Identifier& id) {
        for (auto it = m_sources.begin(); it != m_sources.end(); ++it) {
            if (it->id != id) continue;
            m_sources.erase(it);
            return true;
        }

        return false;
    }

    double measure(const Entity& e, std::vector<LoadPart>* parts = nullptr) const {
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

    std::size_t size() const noexcept { return m_sources.size(); }

private:
    struct Entry {
        Identifier  id;
        std::string name;
        Source      source;
    };

    std::vector<Entry> m_sources;
};

} // namespace voxelspire

#endif // VOXELSPIRE_SURVIVAL_EFFORT_HPP