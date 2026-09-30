#ifndef VOXELSPIRE_ENTITY_BODY_HPP
#define VOXELSPIRE_ENTITY_BODY_HPP

#include <utility>
#include <vector>
#include "../core/identifier.hpp"
#include "player_defaults.hpp"

namespace voxelspire {

namespace Poses {
    inline const Identifier Standing  { "voxelspire:standing" };
    inline const Identifier Crouching { "voxelspire:crouching" };
    inline const Identifier Prone     { "voxelspire:prone" };
    inline const Identifier Swimming  { "voxelspire:swimming" };
} // namespace Poses

struct PoseDimensions {
    double width      = PlayerDefaults::width;
    double height     = PlayerDefaults::height::standing;
    double eye_height = PlayerDefaults::eye_height::standing;
};

class EntityBody {
public:
    explicit EntityBody(PoseDimensions standing) { set(Poses::Standing, standing); }

    EntityBody& set(Identifier pose, PoseDimensions dims) {
        for (auto& entry : m_poses) if (entry.first == pose) { entry.second = dims; return *this; }
        m_poses.emplace_back(pose, dims);
        return *this;
    }

    bool has(Identifier pose) const noexcept { return find(pose) != nullptr; }

    const PoseDimensions& get(Identifier pose) const noexcept {
        const PoseDimensions* d = find(pose);
        return d ? *d : standing();
    }

    const PoseDimensions& standing() const noexcept { return m_poses.front().second; }

    static EntityBody player() {
        EntityBody body({ 
            PlayerDefaults::width,
            PlayerDefaults::height::standing,
            PlayerDefaults::eye_height::standing
        });

        body.set(Poses::Crouching, { 
            PlayerDefaults::width,
            PlayerDefaults::height::crouching,
            PlayerDefaults::eye_height::crouching
        });
        
        body.set(Poses::Prone, { 
            PlayerDefaults::width,
            PlayerDefaults::height::crawling,
            PlayerDefaults::eye_height::crawling
        });

        body.set(Poses::Swimming, {
            PlayerDefaults::width,
            PlayerDefaults::height::swimming,
            PlayerDefaults::eye_height::swimming
        });

        return body;

    }

private:
    const PoseDimensions* find(Identifier pose) const noexcept {
        for (const auto& entry : m_poses) if (entry.first == pose) return &entry.second;
        return nullptr;
    }

    std::vector<std::pair<Identifier, PoseDimensions>> m_poses;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_BODY_HPP