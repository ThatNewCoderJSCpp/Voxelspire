#ifndef VOXELSPIRE_UI_SETTINGS_FILE_HPP
#define VOXELSPIRE_UI_SETTINGS_FILE_HPP

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "color_math.hpp"
#include "settings_model.hpp"

namespace voxelspire {

struct SettingsLoad {
    bool                     found   = false;
    std::uint32_t            applied = Apply::Nothing;
    std::vector<std::string> problems;
};

class SettingsFile {
public:
    static constexpr const char* KEY_SEPARATOR = " | ";
    static constexpr const char* UNBOUND       = "none";
    static constexpr int         CHANNELS      = 4;

    static bool save(const std::vector<SettingsTab>& tabs, const std::string& path) {
        std::ofstream out(path, std::ios::trunc);
        if (!out) return false;

        for (const SettingsTab& tab : tabs) {
            out << "[" << tab.name << "]\n";

            for (const SettingControl& c : tab.controls) {
                if (!c.persist || !c.interactive() || c.kind == ControlKind::Button) continue;
                out << c.key << " = " << value_of(c) << "\n";
            }
            
            out << "\n";
        }

        return static_cast<bool>(out);
    }

    static SettingsLoad load(std::vector<SettingsTab>& tabs, const std::string& path) {
        SettingsLoad result;
        std::ifstream in(path);
        if (!in) return result;
        result.found = true;
        std::unordered_map<std::string, std::string> values;
        std::string line;

        while (std::getline(in, line)) {
            const std::size_t eq = line.find('=');
            if (line.empty() || line[0] == '[' || line[0] == '#' || eq == std::string::npos) continue;
            values[trimmed(line.substr(0, eq))] = trimmed(line.substr(eq + 1));
        }

        for (SettingsTab& tab : tabs)
            for (SettingControl& c : tab.controls) {
                if (!c.persist) continue;
                auto it = values.find(c.key);
                if (it == values.end()) continue;
                std::string problem;

                if (assign(c, it->second, problem)) {
                    result.applied |= c.apply;
                    continue;
                }

                if (c.reset) c.reset();
                result.applied |= c.apply;
                result.problems.push_back(c.label + ": " + problem);
            }

        return result;
    }

private:
    static std::string value_of(const SettingControl& c) {
        switch (c.kind) {
            case ControlKind::Toggle: return c.get_bool() ? "true" : "false";
            case ControlKind::Integer:
            case ControlKind::Decimal: {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.10g", c.get_number());
                return buf;
            }
            case ControlKind::Choice: {
                const std::vector<std::string> options = c.choices();
                const int i = c.get_choice();
                return i >= 0 && static_cast<std::size_t>(i) < options.size() ? options[static_cast<std::size_t>(i)] : std::string();
            }
            case ControlKind::Color: return ColorMath::to_hex(c.get_color(), true);
            case ControlKind::Binding: {
                if (!c.bindings) return UNBOUND;
                std::string out;
                for (const std::string& k : c.bindings->keys(c.action)) out += (out.empty() ? "" : KEY_SEPARATOR) + k;
                return out.empty() ? std::string(UNBOUND) : out;
            }
            default: return std::string();
        }
    }

    static bool assign(SettingControl& c, const std::string& text, std::string& problem) {
        switch (c.kind) {
            case ControlKind::Toggle:
                if (text != "true" && text != "false") { problem = "\"" + text + "\" is not true or false."; return false; }
                c.set_bool(text == "true");
                return true;
            case ControlKind::Integer:
            case ControlKind::Decimal: {
                const NumberCheck check = check_number(c, text);
                if (!check.ok) { problem = check.error; return false; }
                c.set_number(check.value);
                return true;
            }
            case ControlKind::Choice: {
                if (text.empty()) return true;
                const std::vector<std::string> options = c.choices();

                for (std::size_t i = 0; i < options.size(); ++i) {
                    if (options[i] != text) continue;
                    c.set_choice(static_cast<int>(i));
                    return true;
                }

                problem = "\"" + text + "\" is not one of the choices.";
                return false;
            }
            case ControlKind::Color: return assign_color(c, text, problem);
            case ControlKind::Binding: {
                if (!c.bindings) return false;
                InputBindings::Keys keys;

                if (text != UNBOUND) {
                    std::size_t start = 0;
                    const std::string sep = KEY_SEPARATOR;

                    while (start <= text.size()) {
                        const std::size_t at = text.find(sep, start);
                        const std::string key = trimmed(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
                        if (!key.empty()) keys.push_back(key);
                        if (at == std::string::npos) break;
                        start = at + sep.size();
                    }
                }

                c.bindings->bind(c.action, keys);
                return true;
            }
            default: return false;
        }
    }

    static bool assign_color(SettingControl& c, const std::string& text, std::string& problem) {
        Color parsed = c.get_color();

        if (!text.empty() && text[0] == '#') {
            if (!ColorMath::parse_hex(text, true, parsed, parsed, problem)) return false;
            c.set_color(parsed);
            return true;
        }

        int ch[CHANNELS] = { 0, 0, 0, parsed.alpha() };
        std::size_t start = 0;
        int count = 0;

        while (start <= text.size() && count < CHANNELS) {
            const std::size_t comma = text.find(',', start);
            const std::string part = trimmed(text.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
            const NumberCheck check = check_value(part, { 0.0, ColorMath::CHANNEL }, true);
            if (!check.ok) { problem = check.error; return false; }
            ch[count++] = static_cast<int>(check.value);
            if (comma == std::string::npos) break;
            start = comma + 1;
        }

        if (count < CHANNELS - 1) { problem = "\"" + text + "\" needs red, green and blue values."; return false; }
        c.set_color(Color(static_cast<std::uint8_t>(ch[0]), static_cast<std::uint8_t>(ch[1]), static_cast<std::uint8_t>(ch[2]), static_cast<std::uint8_t>(ch[3])));
        return true;
    }
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_FILE_HPP