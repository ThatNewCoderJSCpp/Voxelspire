#include "camera/game_camera.hpp"

namespace voxelspire {

GameCamera::GameCamera(const CameraSettings& settings, unsigned int viewport_w, unsigned int viewport_h, double view_distance) : m_settings(settings),
          m_camera({}, viewport_w, viewport_h, settings.fov_y, settings.near_plane, far_plane_for(view_distance)) {}

void GameCamera::set_settings(const CameraSettings& settings, double view_distance) noexcept {
    m_settings = settings;
    m_camera.set_fov_y(settings.fov_y);
    set_view_distance(view_distance);
}

void GameCamera::update(const Player& player, const World& world, double alpha, double frame_dt) {
    if (m_rigs.empty()) return;
    const vector3d before = m_camera.position();
    m_rigs[m_active]->apply(m_camera, CameraRigContext{ player, world, m_settings, alpha });
    keep_off_surface(world);
    const vector3d now = m_camera.position();
    if (m_has_previous && frame_dt > 0.0) m_velocity = (now - before) / frame_dt;
    m_has_previous = true;
}

void GameCamera::keep_off_surface(const World& world) {
    vector3d p = m_camera.position();
    const double margin = m_settings.near_plane * SURFACE_CLEARANCE;
    const BlockPos cell = BlockPos::containing(p);

    for (int dz = -1; dz <= 1; ++dz) {
        const BlockPos c{ cell.x, cell.y, cell.z + dz };
        const BlockId id = world.block_id_at(c);
        if (!world.blocks().traits(id).fluid || world.block_id_at({ c.x, c.y, c.z + 1 }) == id) continue;
        const double surface = c.z + world.fluid_height(c);
        if (std::fabs(p.z - surface) >= margin) continue;
        p.z = p.z >= surface ? surface + margin : surface - margin;
        m_camera.set_position(p);
        return;
    }
}

} // namespace voxelspire
