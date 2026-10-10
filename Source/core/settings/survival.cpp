#include "core/settings/survival.hpp"

namespace voxelspire {

NeedSettings NeedSettings::thirst() {
    NeedSettings s;
    s.enabled  = false;
    s.max      = PlayerDefaults::thirst::max;
    s.start    = PlayerDefaults::thirst::start;
    s.drain    = PlayerDefaults::thirst::drain;
    s.damage   = PlayerDefaults::thirst::damage;
    s.interval = PlayerDefaults::thirst::interval;
    return s;
}

} // namespace voxelspire
