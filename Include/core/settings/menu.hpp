#ifndef VOXELSPIRE_CORE_SETTINGS_MENU_HPP
#define VOXELSPIRE_CORE_SETTINGS_MENU_HPP

#include <string>
#include "../types.hpp"

namespace voxelspire {

struct MenuSettings {
    bool        pause_game    = true;
    bool        save_on_close = true;
    double      scale         = 1.0;
    std::string file          = "config/voxelspire_settings.cfg";
    Color       backdrop      { 0, 0, 0, 150 };
    Color       panel         { 20, 23, 31, 245 };
    Color       sidebar       { 14, 16, 22, 255 };
    Color       row_hover     { 255, 255, 255, 14 };
    Color       text          { 232, 236, 244 };
    Color       muted         { 140, 150, 168 };
    Color       accent        { 92, 164, 255 };
    Color       toggle_on     { 64, 186, 104 };
    Color       control       { 42, 47, 60 };
    Color       danger        { 214, 88, 88 };
    Color       error         { 255, 96, 96 };
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_MENU_HPP