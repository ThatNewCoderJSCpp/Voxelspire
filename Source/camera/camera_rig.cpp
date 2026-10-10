#include "camera/camera_rig.hpp"

namespace voxelspire {

double CameraRig::clear_distance(const CameraRigContext& ctx, const vector3d& from, const vector3d& dir, double wanted) {
    if (auto hit = raycast_blocks(ctx.world, from, dir, wanted + ctx.settings.collision_margin))
        return vmax(0.1, hit->distance - ctx.settings.collision_margin);
    return wanted;
}

void FirstPersonRig::apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) {
    camera.set_position(ctx.player.eye_position(ctx.alpha));
    camera.look_in(ctx.player.look_direction(), WORLD_UP);
}

void ThirdPersonBackRig::apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) {
    const vector3d pivot = ctx.player.eye_position(ctx.alpha);
    const vector3d back = -ctx.player.look_direction();
    const double dist = clear_distance(ctx, pivot, back, ctx.settings.third_person_distance);
    camera.set_position(pivot + back * dist);
    camera.look_in(-back, WORLD_UP);
}

void ThirdPersonFrontRig::apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) {
    const vector3d pivot = ctx.player.eye_position(ctx.alpha);
    const vector3d fwd = ctx.player.look_direction();
    const double dist = clear_distance(ctx, pivot, fwd, ctx.settings.third_person_distance);
    camera.set_position(pivot + fwd * dist);
    camera.look_in(-fwd, WORLD_UP);
}

} // namespace voxelspire
