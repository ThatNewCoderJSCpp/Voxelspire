#include "ui/settings_model.hpp"

namespace voxelspire {

Bounds AltUnit::bounds(Bounds b) const noexcept {
    const double a = to(b.min), c = to(b.max), ua = to(b.usual_min), uc = to(b.usual_max);
    return { vmin(a, c), vmax(a, c), vmin(ua, uc), vmax(ua, uc) };
}

Bounds SettingControl::limits() const {
    if (!live_limits) return number.limits;
    const Bounds b = live_limits();
    return { b.min, vmax(b.max, b.min) };
}

std::string trimmed(const std::string& s) {
    const std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string();
    const std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string format_number(double v, int decimals) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", vmax(decimals, 0), v);
    std::string s = buf;
    if (s == "-0" || s.find_first_not_of("-0.") == std::string::npos) s.erase(0, s[0] == '-' ? 1 : 0);
    return s;
}

bool looks_numeric(const std::string& text) noexcept {
    std::size_t i = text.empty() || text[0] != '-' ? 0 : 1;
    int digits = 0, dots = 0;

    for (; i < text.size(); ++i) {
        const char ch = text[i];
        if (std::isdigit(static_cast<unsigned char>(ch))) ++digits;
        else if (ch == '.') ++dots;
        else return false;
    }

    return digits > 0 && dots <= 1;
}

NumberCheck check_value(const std::string& raw, Bounds b, bool whole) {
    NumberCheck out;
    const std::string text = trimmed(raw);
    const std::string quoted = "\"" + text + "\"";
    if (text.empty()) { out.error = "Nothing was entered."; return out; }
    if (!looks_numeric(text)) { out.error = quoted + " is not a number."; return out; }
    if (whole && text.find('.') != std::string::npos) { out.error = quoted + " is not a whole number."; return out; }
    const double v = std::strtod(text.c_str(), nullptr);
    if (!std::isfinite(v)) { out.error = quoted + " is not a number."; return out; }
    if (v < b.min) { out.error = quoted + " is below the minimum of " + format_limit(b.min) + "."; return out; }
    if (v > b.max) { out.error = quoted + " is above the maximum of " + format_limit(b.max) + "."; return out; }
    out.ok = true;
    out.value = v;
    return out;
}

std::string slug(const std::string& text) {
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

SettingsPage& SettingsPage::header(const std::string& label) {
    SettingControl c;
    c.kind  = ControlKind::Header;
    c.label = label;
    m_section = slug(label);
    m_tab.controls.push_back(std::move(c));
    return *this;
}

SettingsPage& SettingsPage::custom_toggle(
    const std::string& label,
    const std::string& description,
    std::function<bool()> get,
    std::function<void(bool)> set
) {
    SettingControl c = base(ControlKind::Toggle, label, description);
    c.get_bool = std::move(get);
    c.set_bool = std::move(set);
    return add(std::move(c));
}

SettingsPage& SettingsPage::custom_integer(
    const std::string& label, 
    const std::string& description, 
    std::function<double()> get,
    std::function<void(double)> set, 
    Bounds limits
) {
    return custom_number(ControlKind::Integer, label, description, std::move(get), std::move(set), limits);
}

SettingsPage& SettingsPage::custom_decimal(
    const std::string& label, 
    const std::string& description, 
    std::function<double()> get,
    std::function<void(double)> set, 
    Bounds limits
) {
    return custom_number(ControlKind::Decimal, label, description, std::move(get), std::move(set), limits);
}

SettingsPage& SettingsPage::custom_choice(
    const std::string& label, 
    const std::string& description, 
    std::function<std::vector<std::string>()> options,
    std::function<int()> get, 
    std::function<void(int)> set
) {
    SettingControl c = base(ControlKind::Choice, label, description);
    c.options_source = std::move(options);
    c.get_choice     = std::move(get);
    c.set_choice     = std::move(set);
    return add(std::move(c));
}

SettingsPage& SettingsPage::binding(const ActionInfo& info) {
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

SettingsPage& SettingsPage::button(const std::string& label, const std::string& description, std::function<void()> press) {
    SettingControl c = base(ControlKind::Button, label, description);
    c.persist = false;
    c.press   = std::move(press);
    return add(std::move(c));
}

SettingsPage& SettingsPage::defaults(std::function<void()> reset, std::function<bool()> is_default) {
    SettingControl& c = last();
    c.reset      = std::move(reset);
    c.is_default = std::move(is_default);
    if (m_touch) c.reset = [same = c.is_default, set = c.reset, touch = m_touch] { if (same && same()) return; set(); touch(); };
    return *this;
}

SettingsPage& SettingsPage::custom_number(
    ControlKind kind, 
    const std::string& label, 
    const std::string& description, 
    std::function<double()> get,
    std::function<void(double)> set, 
    Bounds limits
) {
    SettingControl c = base(kind, label, description);
    const bool whole = kind == ControlKind::Integer;
    c.number.limits = limits;
    c.get_number    = std::move(get);
    c.set_number    = [set = std::move(set), limits, whole](double v) { const double k = limits.clamp(v); set(whole ? std::round(k) : k); };
    return add(std::move(c));
}

SettingControl SettingsPage::base(ControlKind kind, const std::string& label, const std::string& description) const {
    SettingControl c;
    c.kind        = kind;
    c.label       = label;
    c.description = description;
    c.apply       = m_apply;
    c.active      = m_active;
    c.key         = slug(m_tab.id) + "." + (m_section.empty() ? std::string() : m_section + ".") + slug(label);
    return c;
}

void SettingsPage::wrap(SettingControl& c, const std::function<void()>& touch) {
    if (c.set_bool)   c.set_bool   = [get = c.get_bool, set = c.set_bool, touch](bool v) { if (get() == v) return; set(v); touch(); };
    if (c.set_number) c.set_number = [get = c.get_number, set = c.set_number, touch](double v) { const double before = get(); set(v); if (get() != before) touch(); };
    if (c.set_choice) c.set_choice = [get = c.get_choice, set = c.set_choice, touch](int v) { if (get() == v) return; set(v); touch(); };
    if (c.set_color)  c.set_color  = [get = c.get_color, set = c.set_color, touch](Color v) { if (get() == v) return; set(v); touch(); };
    if (c.set_text)   c.set_text   = [get = c.get_text, set = c.set_text, touch](std::string v) { if (get() == v) return; set(std::move(v)); touch(); };
    if (c.reset)      c.reset      = [same = c.is_default, set = c.reset, touch] { if (same && same()) return; set(); touch(); };
}

} // namespace voxelspire
