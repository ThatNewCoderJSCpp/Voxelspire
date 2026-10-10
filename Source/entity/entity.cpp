#include "entity/entity.hpp"

namespace voxelspire {

AABB Entity::bounding_box_at(const vector3d& feet) const noexcept {
    const double h = m_width * 0.5;
    return { { feet.x - h, feet.y - h, feet.z }, { feet.x + h, feet.y + h, feet.z + m_height } };
}

void Entity::set_look(double yaw_degrees, double pitch_degrees) noexcept {
    m_yaw = std::remainder(yaw_degrees, 360.0);
    m_pitch = vclamp(pitch_degrees, -MAX_PITCH, MAX_PITCH);
}

vector3d Entity::look_direction() const noexcept {
    const double y = deg_to_rad(m_yaw), p = deg_to_rad(m_pitch);
    return { -std::sin(y) * std::cos(p), std::cos(y) * std::cos(p), std::sin(p) };
}

} // namespace voxelspire
