#ifndef VOXELSPIRE_ENTITY_BODY_HPP
#define VOXELSPIRE_ENTITY_BODY_HPP

#include <string>
#include <unordered_map>
#include <utility>

namespace voxelspire {

namespace Poses {
    inline const std::string Standing  = "voxelspire:standing";
    inline const std::string Crouching = "voxelspire:crouching";
    inline const std::string Prone     = "voxelspire:prone";
} // namespace Poses

constexpr double PLAYER_DEFAULT_WIDTH      = 0.64;
constexpr double PLAYER_DEFAULT_HEIGHT     = PLAYER_DEFAULT_WIDTH * 3;
constexpr double PLAYER_DEFUALT_EYE_HEIGHT = PLAYER_DEFAULT_HEIGHT / 1.2;

struct PoseDimensions {
    double width      = PLAYER_DEFAULT_WIDTH;
    double height     = PLAYER_DEFAULT_HEIGHT;
    double eye_height = PLAYER_DEFUALT_EYE_HEIGHT;
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
        EntityBody body({ PLAYER_DEFAULT_WIDTH, PLAYER_DEFAULT_HEIGHT, PLAYER_DEFUALT_EYE_HEIGHT });
        body.set(Poses::Crouching, { PLAYER_DEFAULT_WIDTH, PLAYER_DEFAULT_HEIGHT * (1.0 - 0.15), PLAYER_DEFUALT_EYE_HEIGHT * (1.0 - 0.30)});
        body.set(Poses::Prone,     { PLAYER_DEFAULT_WIDTH, PLAYER_DEFAULT_HEIGHT * (1.0 - 0.80), PLAYER_DEFUALT_EYE_HEIGHT * (1.0 - 0.90)});
        return body;
    }

private:
    std::unordered_map<std::string, PoseDimensions> m_poses;
};

} // namespace voxelspire

#endif // VOXELSPIRE_ENTITY_BODY_HPP