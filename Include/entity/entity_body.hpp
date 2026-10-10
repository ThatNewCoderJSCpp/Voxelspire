#ifndef VOXELSPIRE_ENTITY_BODY_HPP
#define VOXELSPIRE_ENTITY_BODY_HPP

#include <utility>
#include <vector>
#include "../core/identifier.hpp"
#include "player_defaults.hpp"

namespace voxelspire {

namespace Poses {
    inline const Identifier Standing  = core_id(Kind::Pose, "standing");
    inline const Identifier Crouching = core_id(Kind::Pose, "crouching");
    inline const Identifier Prone     = core_id(Kind::Pose, "prone");
    inline const Identifier Swimming  = core_id(Kind::Pose, "swimming");
} // namespace Poses

struct PoseDimensions {
    double width      = PlayerDefaults::width;
    double height     = PlayerDefaults::height::standing;
    double eye_height = PlayerDefaults::eye_height::standing;
};

class EntityBody {
public:
    explicit EntityBody(PoseDimensions standing) { set(Poses::Standing, standing); }

    EntityBody& set(Identifier pose, PoseDimensions dims);

    bool has(Identifier pose) const noexcept { return find(pose) != nullptr; }

    const PoseDimensions& get(Identifier pose) const noexcept {
        const PoseDimensions* d = find(pose);
        return d ? *d : standing();
    }

    const PoseDimensions& standing() const noexcept { return m_poses.front().second; }

    static EntityBody player();

private:
    const PoseDimensions* find(Identifier pose) const noexcept {
        for (const auto& entry : m_poses) if (entry.first == pose) return &entry.second;
        return nullptr;
    }

    std::vector<std::pair<Identifier, PoseDimensions>> m_poses;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_BODY_HPP