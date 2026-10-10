#ifndef VOXELSPIRE_RENDER_VITALS_HUD_HPP
#define VOXELSPIRE_RENDER_VITALS_HUD_HPP

#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include "../core/settings.hpp"
#include "../survival/player_survival.hpp"

namespace voxelspire {

struct VitalsContext {
    std::string     activity;
    bool            heat_shown       = false;
    double          body_temperature = 0.0;
    std::string     body_state;
    TemperatureUnit unit             = TemperatureUnit::Celsius;
    double          now              = 0.0;
    std::string     close_key;
    std::string     respawn_key;
    std::string     drink_key;
    bool            water_in_reach   = false;
};

struct VitalsArea {
    int x = 0, y = 0, w = 0, h = 0;
};

class VitalsHud {
public:
    static constexpr double HEART_SIZE   = 18.0;
    static constexpr int    PER_ROW      = 10;
    static constexpr double HEART_STEP   = 1.1;
    static constexpr double ROW_STEP     = 0.9;
    static constexpr double GROUP_GAP    = 2.0;
    static constexpr double BAR_HEIGHT   = 0.3;
    static constexpr double BAR_GAP      = 0.12;
    static constexpr double OUTLINE      = 1.5;
    static constexpr double FULL_SHARE   = 0.75;
    static constexpr double HALF_SHARE   = 0.25;
    static constexpr double PULSE_SPEED  = 6.0;
    static constexpr double BUBBLE_SHARE = 0.32;
    static constexpr int    HEART_POINTS = 40;
    static constexpr double CURVE_X      = 16.0;
    static constexpr double CURVE_TOP    = 12.0;
    static constexpr double CURVE_BOTTOM = 17.0;
    static constexpr double CURVE_A      = 13.0;
    static constexpr double CURVE_B      = 5.0;
    static constexpr double CURVE_C      = 2.0;
    static constexpr double CURVE_D      = 1.0;
    static constexpr double EPSILON      = 1e-9;

    void update(double dt, const PlayerSurvival& ps);

    void render(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs, const PlayerSurvival& ps, double now);

    void render_panel(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs, const PlayerSurvival& ps, const VitalsContext& ctx, const VitalsArea* area = nullptr);

    void render_death(fizmo::windows::Renderer& r, unsigned int w, unsigned int h, const HudSettings& hs, const PlayerSurvival& ps, const VitalsContext& ctx);

private:
    static constexpr std::size_t VITALS           = Survival::VITALS;
    static constexpr double      HALF             = 0.5;
    static constexpr double      TITLE_SCALE      = 1.35;
    static constexpr double      DEATH_SCALE      = 2.5;
    static constexpr double      DEATH_RISE       = 1.5;
    static constexpr double      HINT_DROP        = 2.0;
    static constexpr double      PANEL_PADDING    = 1.6;
    static constexpr double      PANEL_BAR        = 9.0;
    static constexpr double      PANEL_BAR_HEIGHT = 0.45;
    static constexpr double      SECONDS_PER_MIN  = 60.0;
    static constexpr double      FAHRENHEIT_SCALE = 1.8;
    static constexpr double      FAHRENHEIT_ZERO  = 32.0;
    static constexpr double      RECENT_SECONDS   = 60.0;
    static constexpr std::size_t TEXT_BUFFER      = 160;
    static constexpr const char* TITLE            = "Status";
    static constexpr const char* DEATH_TITLE      = "You died";

    struct Row {
        std::string label;
        std::string value;
        double      fill   = -1.0;
        Color       color;
        bool        header = false;
    };

    static int px(double v) noexcept { return static_cast<int>(std::lround(v)); }

    static std::string format(const char* f, ...);

    static fizmo::text::TextStyle style(const HudSettings& hs, double size, const Color& c, bool bold = false);

    static int text_width(fizmo::windows::Renderer& r, const std::string& text, const fizmo::text::TextStyle& st) {
        return static_cast<int>(r.measure_text(text, st).width);
    }

    bool visible(VitalsShown mode, Vital v, const VitalsHudSettings& vs) const noexcept;

    static std::vector<fizmo::windows::RenderPoint> heart_shape(double x, double y, double size, bool half);

    double hearts(fizmo::windows::Renderer& r, const PlayerSurvival& ps, const VitalsHudSettings& vs, double left, double base, double size, double now) const;

    void bars(fizmo::windows::Renderer& r, const PlayerSurvival& ps, const VitalsHudSettings& vs, double x, double base, double size, double width) const;

    void bubbles(fizmo::windows::Renderer& r, const PlayerSurvival& ps, const VitalsHudSettings& vs, double x, double y, double size) const;

    static std::string rate_text(double per_min, bool per_second);

    std::string temperature(double c, TemperatureUnit unit) const;

    void meter(const Survival& v, const SurvivalSettings& s, Vital vital, const char* name, const char* unit, bool per_second, const Color& color, const std::string& note);

    void line(const std::string& label, const std::string& value) { m_rows.push_back({ label, value }); }
    void header(const std::string& label) { Row r; r.label = label; r.header = true; m_rows.push_back(r); }

    static const char* need_note(const NeedSettings& n, double value, bool lacking);

    void build_rows(const PlayerSurvival& ps, const VitalsContext& ctx, const VitalsHudSettings& vs);

    static std::string join(const std::vector<std::string>& parts) {
        std::string out;
        for (const std::string& p : parts) out += (out.empty() ? "" : ", ") + p;
        return out;
    }

    static std::vector<std::string> effect_list(const PlayerSurvival& ps);

    static constexpr double LONG_AGO = 1.0e9;

    std::array<double, VITALS> m_shown_for = every(LONG_AGO);

    static std::array<double, VITALS> every(double v) noexcept {
        std::array<double, VITALS> a{};
        a.fill(v);
        return a;
    }
    std::vector<Row>           m_rows;
};

} // namespace voxelspire

#endif // VOXELSPIRE_RENDER_VITALS_HUD_HPP