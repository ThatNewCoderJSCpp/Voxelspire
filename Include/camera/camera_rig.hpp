#ifndef VOXELSPIRE_CAMERA_RIG_HPP
#define VOXELSPIRE_CAMERA_RIG_HPP

#include "../core/settings.hpp"
#include "../entity/player.hpp"
#include "../world/voxel_raycast.hpp"

namespace voxelspire {

struct CameraRigContext {
    const Player&         player;
    const World&          world;
    const CameraSettings& settings;
    double                alpha;
};

class CameraRig {
public:
    virtual ~CameraRig() = default;
    virtual const char* name() const noexcept = 0;
    virtual bool shows_player() const noexcept { return true; }
    virtual void apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) = 0;

protected:
    static constexpr vector3d WORLD_UP{ 0.0, 0.0, 1.0 };

    static double clear_distance(const CameraRigContext& ctx, const vector3d& from, const vector3d& dir, double wanted);
};

class FirstPersonRig final : public CameraRig {
public:
    const char* name() const noexcept override { return "First person"; }
    bool shows_player() const noexcept override { return false; }

    void apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) override;
};

class ThirdPersonBackRig final : public CameraRig {
public:
    const char* name() const noexcept override { return "Third person (back)"; }

    void apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) override;
};

class ThirdPersonFrontRig final : public CameraRig {
public:
    const char* name() const noexcept override { return "Third person (front)"; }

    void apply(fizmo::graphics::Camera3D& camera, const CameraRigContext& ctx) override;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CAMERA_RIG_HPP