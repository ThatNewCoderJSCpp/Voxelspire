#ifndef VOXELSPIRE_CORE_SETTINGS_HUD_HPP
#define VOXELSPIRE_CORE_SETTINGS_HUD_HPP

#include <cstdint>
#include "../types.hpp"

namespace voxelspire {

enum class HudCorner : std::uint8_t { TopLeft = 0, TopRight, BottomLeft, BottomRight };

struct HudSections {
    bool performance = true;
    bool gpu         = true;
    bool player      = true;
    bool world       = true;
    bool rendering   = true;
    bool lighting    = true;
};

enum class VitalsShown : std::uint8_t { Always = 0, WhenChanging, Never };

struct VitalsHudSettings {
    bool        show          = true;
    bool        hearts        = true;
    VitalsShown hunger        = VitalsShown::Always;
    VitalsShown thirst        = VitalsShown::Always;
    VitalsShown stamina       = VitalsShown::WhenChanging;
    VitalsShown breath        = VitalsShown::WhenChanging;
    double      scale         = 1.0;
    double      bottom        = 56.0;
    double      linger        = 2.0;
    double      low_flash     = 0.25;
    Color       heart         { 225, 45, 55 };
    Color       heart_empty   { 40, 18, 22, 200 };
    Color       heart_outline { 0, 0, 0, 220 };
    Color       hunger_color  { 220, 145, 60 };
    Color       thirst_color  { 70, 150, 240 };
    Color       stamina_color { 150, 220, 90 };
    Color       breath_color  { 200, 235, 255 };
    Color       bar_back      { 0, 0, 0, 150 };
    Color       panel_back    { 10, 12, 20, 225 };
    Color       death_tint    { 120, 0, 0, 110 };
};

struct HudSettings {
    bool        show_debug       = true;
    bool        show_crosshair   = true;
    bool        show_last_key    = true;
    HudCorner   corner           = HudCorner::TopLeft;
    HudCorner   hint_corner      = HudCorner::BottomLeft;
    double      scale            = 1.0;
    double      text_size        = 15.0;
    double      hint_text_size   = 13.0;
    double      line_spacing     = 1.3;
    double      section_spacing  = 0.5;
    double      column_gap       = 14.0;
    double      margin           = 12.0;
    double      padding          = 10.0;
    double      refresh_interval = 0.25;
    bool        bold_headers     = true;
    bool        fit_to_screen    = true;
    Color       text_color       { 238, 242, 250 };
    Color       label_color      { 150, 172, 205 };
    Color       header_color     { 255, 208, 110 };
    Color       background       { 8, 10, 16, 185 };
    Color       text_shadow      { 0, 0, 0, 210 };
    double      shadow_offset    = 1.0;
    Color       crosshair_color  { 255, 255, 255, 220 };
    double      crosshair_size   = 8.0;
    double      crosshair_gap    = 0.0;
    double      crosshair_thickness = 2.0;
    double      meter_width      = 5.0;
    Color       meter_background { 255, 255, 255, 38 };
    Color       sky_meter        { 125, 195, 255 };
    Color       level_meter      { 255, 212, 96 };
    Color       red_meter        { 235, 80, 80 };
    Color       green_meter      { 90, 210, 110 };
    Color       blue_meter       { 95, 145, 255 };
    HudSections sections;
    VitalsHudSettings vitals;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_SETTINGS_HUD_HPP