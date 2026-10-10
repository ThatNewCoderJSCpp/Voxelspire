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

    ActivityCost cost(const Identifier& activity, const ExertionSettings& s) const;

    static ExertionTable defaults();

    static bool rests_when_still(const Identifier& activity) noexcept;

private:
    struct Entry {
        Identifier   activity;
        Member       member = nullptr;
        ActivityCost cost;
        bool         fixed  = false;
    };

    Entry& entry(const Identifier& activity);

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

    bool remove(const Identifier& id);

    double measure(const Entity& e, std::vector<LoadPart>* parts = nullptr) const;

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