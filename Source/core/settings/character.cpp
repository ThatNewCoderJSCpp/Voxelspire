#include "core/settings/character.hpp"

namespace voxelspire {

EntityBody CharacterSettings::body() const {
    EntityBody b({ width, standing_height, standing_eye_height });
    b.set(Poses::Crouching, { width, crouching_height, crouching_eye_height });
    b.set(Poses::Prone,     { width, crawling_height,  crawling_eye_height });
    b.set(Poses::Swimming,  { width, swimming_height,  swimming_eye_height });
    return b;
}

} // namespace voxelspire
