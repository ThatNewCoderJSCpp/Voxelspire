#ifndef VOXELSPIRE_UI_SETTINGS_MODEL_HPP
#define VOXELSPIRE_UI_SETTINGS_MODEL_HPP

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../core/settings.hpp"

namespace voxelspire {

struct Apply {
    static constexpr std::uint32_t Nothing     = 0;
    static constexpr std::uint32_t Display     = 1u << 0;
    static constexpr std::uint32_t Camera      = 1u << 1;
    static constexpr std::uint32_t Character   = 1u << 2;
    static constexpr std::uint32_t Physics     = 1u << 3;
    static constexpr std::uint32_t Sky         = 1u << 4;
    static constexpr std::uint32_t Lighting    = 1u << 5;
    static constexpr std::uint32_t LightFormat = 1u << 6;
    static constexpr std::uint32_t Terrain     = 1u << 7;
    static constexpr std::uint32_t Streaming   = 1u << 8;
    static constexpr std::uint32_t Particles   = 1u << 9;
    static constexpr std::uint32_t Entities    = 1u << 10;
    static constexpr std::uint32_t HandLight   = 1u << 11;
    static constexpr std::uint32_t Player      = 1u << 12;
    static constexpr std::uint32_t Celestial   = 1u << 13;
    static constexpr std::uint32_t Everything  = ~0u;
};

enum class ControlKind : std::uint8_t { Header = 0, Toggle, Number, Choice, Color, Binding, Button };

struct NumberRange {
    double      min      = 0.0;
    double      max      = 1.0;
    double      step     = 0.1;
    int         decimals = 2;
    const char* unit     = "";
    bool        log      = false;
};

struct SettingControl {
    ControlKind   kind = ControlKind::Header;
    std::string   key;
    std::string   label;
    std::string   description;
    std::uint32_t apply   = Apply::Nothing;
    bool          persist = true;
    std::function<bool()> active;

    std::function<bool()>       get_bool;
    std::function<void(bool)>   set_bool;
    std::function<double()>     get_number;
    std::function<void(double)> set_number;
    NumberRange                 range;
    std::function<double()>     live_max;
    std::vector<std::string>    options;
    std::function<int()>        get_choice;
    std::function<void(int)>    set_choice;
    std::function<Color()>      get_color;
    std::function<void(Color)>  set_color;
    bool                        with_alpha = false;
    Action                      action = Action::MoveForward;
    InputBindings*              bindings = nullptr;
    std::function<void()>       press;

    std::function<void()> reset;
    std::function<bool()> is_default;

    bool enabled() const { return !active || active(); }

    NumberRange limits() const {
        NumberRange r = range;
        if (live_max) r.max = vmax(live_max(), r.min);
        return r;
    }
    
    bool interactive() const noexcept { return kind != ControlKind::Header; }
};

struct SettingsTab {
    std::string                 name;
    std::string                 summary;
    std::vector<SettingControl> controls;
};

template <typename A>
auto field(A GameSettings::*a) {
    return [a](GameSettings& g) -> A& { return g.*a; };
}

template <typename A, typename B>
auto field(A GameSettings::*a, B A::*b) {
    return [a, b](GameSettings& g) -> B& { return (g.*a).*b; };
}

template <typename A, typename B, typename C>
auto field(A GameSettings::*a, B A::*b, C B::*c) {
    return [a, b, c](GameSettings& g) -> C& { return ((g.*a).*b).*c; };
}

inline std::string slug(const std::string& text) {
    std::string out;
    bool gap = false;

    for (char ch : text) {
        const unsigned char c = static_cast<unsigned char>(ch);

        if (std::isalnum(c)) {
            if (gap && !out.empty()) out += '_';
            out += static_cast<char>(std::tolower(c));
            gap = false;
        } else {
            gap = true;
        }
    }

    return out;
}

class SettingsPage {
public:
    SettingsPage(SettingsTab& tab, GameSettings& live, GameSettings& defaults) : m_tab(tab), m_live(live), m_defaults(defaults) {}

    SettingsPage& applies(std::uint32_t groups) { m_apply = groups; return *this; }
    SettingsPage& when(std::function<bool()> active) { m_active = std::move(active); return *this; }
    SettingsPage& always() { m_active = {}; return *this; }
    SettingsPage& touching(std::function<void()> touch) { m_touch = std::move(touch); return *this; }

    SettingsPage& header(const std::string& label) {
        SettingControl c;
        c.kind  = ControlKind::Header;
        c.label = label;
        m_section = slug(label);
        m_tab.controls.push_back(std::move(c));
        return *this;
    }

    template <typename F>
    SettingsPage& toggle(const std::string& label, const std::string& description, F at) {
        SettingControl c = base(ControlKind::Toggle, label, description);
        GameSettings* live = &m_live;
        GameSettings* defaults = &m_defaults;
        c.get_bool   = [live, at] { return static_cast<bool>(at(*live)); };
        c.set_bool   = [live, at](bool v) { at(*live) = v; };
        c.reset      = [live, defaults, at] { at(*live) = at(*defaults); };
        c.is_default = [live, defaults, at] { return at(*live) == at(*defaults); };
        return add(std::move(c));
    }

    template <typename F>
    SettingsPage& number(const std::string& label, const std::string& description, F at, NumberRange range) {
        SettingControl c = base(ControlKind::Number, label, description);
        GameSettings* live = &m_live;
        GameSettings* defaults = &m_defaults;
        using T = std::remove_reference_t<decltype(at(*live))>;
        c.range      = range;
        c.get_number = [live, at] { return static_cast<double>(at(*live)); };
        c.set_number = [live, at](double v) { at(*live) = convert<T>(v); };
        c.reset      = [live, defaults, at] { at(*live) = at(*defaults); };
        c.is_default = [live, defaults, at] { return same(static_cast<double>(at(*live)), static_cast<double>(at(*defaults))); };
        return add(std::move(c));
    }

    template <typename F>
    SettingsPage& choice(const std::string& label, const std::string& description, F at, std::vector<std::string> options) {
        SettingControl c = base(ControlKind::Choice, label, description);
        GameSettings* live = &m_live;
        GameSettings* defaults = &m_defaults;
        using T = std::remove_reference_t<decltype(at(*live))>;
        c.options    = std::move(options);
        c.get_choice = [live, at] { return static_cast<int>(at(*live)); };
        c.set_choice = [live, at](int v) { at(*live) = static_cast<T>(v); };
        c.reset      = [live, defaults, at] { at(*live) = at(*defaults); };
        c.is_default = [live, defaults, at] { return at(*live) == at(*defaults); };
        return add(std::move(c));
    }

    template <typename F>
    SettingsPage& color(const std::string& label, const std::string& description, F at, bool with_alpha = false) {
        SettingControl c = base(ControlKind::Color, label, description);
        GameSettings* live = &m_live;
        GameSettings* defaults = &m_defaults;
        c.with_alpha = with_alpha;
        c.get_color  = [live, at] { return at(*live); };
        c.set_color  = [live, at](Color v) { at(*live) = v; };
        c.reset      = [live, defaults, at] { at(*live) = at(*defaults); };
        c.is_default = [live, defaults, at] { return at(*live) == at(*defaults); };
        return add(std::move(c));
    }

    SettingsPage& binding(const ActionInfo& info) {
        SettingControl c = base(ControlKind::Binding, info.label, info.description);
        c.key      = "bindings." + std::string(info.key);
        c.action   = info.action;
        c.bindings = &m_live.bindings;
        InputBindings* live = &m_live.bindings;
        InputBindings* defaults = &m_defaults.bindings;
        const Action a = info.action;
        c.reset      = [live, defaults, a] { live->bind(a, defaults->keys(a)); };
        c.is_default = [live, defaults, a] { return live->keys(a) == defaults->keys(a); };
        return add(std::move(c));
    }

    SettingsPage& custom_number(
        const std::string& label, 
        const std::string& description, 
        std::function<double()> get,
        std::function<void(double)> set, 
        NumberRange range, 
        bool persist = true
    ) {
        SettingControl c = base(ControlKind::Number, label, description);
        c.range      = range;
        c.persist    = persist;
        c.get_number = std::move(get);
        c.set_number = std::move(set);
        return add(std::move(c));
    }

    SettingsPage& custom_choice(
        const std::string& label, 
        const std::string& description, 
        std::vector<std::string> options,
        std::function<int()> get, 
        std::function<void(int)> set, 
        bool persist = true
    ) {
        SettingControl c = base(ControlKind::Choice, label, description);
        c.options    = std::move(options);
        c.persist    = persist;
        c.get_choice = std::move(get);
        c.set_choice = std::move(set);
        return add(std::move(c));
    }

    SettingsPage& button(const std::string& label, const std::string& description, std::function<void()> press) {
        SettingControl c = base(ControlKind::Button, label, description);
        c.persist = false;
        c.press   = std::move(press);
        return add(std::move(c));
    }

    GameSettings& live() noexcept { return m_live; }

private:
    static constexpr double SAME_TOLERANCE = 1e-7;

    static bool same(double a, double b) noexcept { return std::fabs(a - b) <= SAME_TOLERANCE * vmax(1.0, std::fabs(b)); }

    template <typename T>
    static T convert(double v) {
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>) return static_cast<T>(std::llround(vmax(v, 0.0)));
            else return static_cast<T>(std::llround(v));
        } else {
            return static_cast<T>(v);
        }
    }

    SettingControl base(ControlKind kind, const std::string& label, const std::string& description) const {
        SettingControl c;
        c.kind        = kind;
        c.label       = label;
        c.description = description;
        c.apply       = m_apply;
        c.active      = m_active;
        c.key         = slug(m_tab.name) + "." + (m_section.empty() ? std::string() : m_section + ".") + slug(label);
        return c;
    }

    SettingsPage& add(SettingControl c) {
        if (m_touch) wrap(c, m_touch);
        m_tab.controls.push_back(std::move(c));
        return *this;
    }

    static void wrap(SettingControl& c, const std::function<void()>& touch) {
        if (c.set_bool)   c.set_bool   = [get = c.get_bool, set = c.set_bool, touch](bool v) { if (get() == v) return; set(v); touch(); };
        if (c.set_number) c.set_number = [get = c.get_number, set = c.set_number, touch](double v) { const double before = get(); set(v); if (get() != before) touch(); };
        if (c.set_choice) c.set_choice = [get = c.get_choice, set = c.set_choice, touch](int v) { if (get() == v) return; set(v); touch(); };
        if (c.set_color)  c.set_color  = [get = c.get_color, set = c.set_color, touch](Color v) { if (get() == v) return; set(v); touch(); };
        if (c.reset)      c.reset      = [same = c.is_default, set = c.reset, touch] { if (same && same()) return; set(); touch(); };
    }

    SettingsTab&          m_tab;
    GameSettings&         m_live;
    GameSettings&         m_defaults;
    std::uint32_t         m_apply = Apply::Nothing;
    std::function<bool()> m_active;
    std::function<void()> m_touch;
    std::string           m_section;
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_MODEL_HPP