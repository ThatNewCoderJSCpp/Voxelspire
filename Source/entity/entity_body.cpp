#include "entity/entity_body.hpp"

namespace voxelspire {

EntityBody& EntityBody::set(Identifier pose, PoseDimensions dims) {
    for (auto& entry : m_poses) if (entry.first == pose) { entry.second = dims; return *this; }
    m_poses.emplace_back(pose, dims);
    return *this;
}

EntityBody EntityBody::player() {
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

} // namespace voxelspire
