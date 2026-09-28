#ifndef VOXELSPIRE_ENTITY_BODY_HPP
#define VOXELSPIRE_ENTITY_BODY_HPP

#include <string>
#include <unordered_map>
#include <utility>
#include "player_defaults.hpp"

namespace voxelspire {

namespace Poses {
    inline const std::string Standing  = "voxelspire:standing";
    inline const std::string Crouching = "voxelspire:crouching";
    inline const std::string Prone     = "voxelspire:prone";
} // namespace Poses

struct PoseDimensions {
    double width      = PlayerDefaults::width;
    double height     = PlayerDefaults::height::standing;
    double eye_height = PlayerDefaults::eye_height::standing;
};

class EntityBody {
public:
    explicit EntityBody(PoseDimensions standing) { set(Poses::Standing, standing); }

    EntityBody& set(std::string pose, PoseDimensions dims) { m_poses[std::move(pose)] = dims; return *this; }

    bool has(const std::string& pose) const noexcept { return m_poses.count(pose) > 0; }

    const PoseDimensions& get(const std::string& pose) const noexcept {
        auto it = m_poses.find(pose);
        return it != m_poses.end() ? it->second : m_poses.at(Poses::Standing);
    }

    const PoseDimensions& standing() const noexcept { return m_poses.at(Poses::Standing); }

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

        return body;

    }

private:
    std::unordered_map<std::string, PoseDimensions> m_poses;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_BODY_HPP