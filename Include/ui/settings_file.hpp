#ifndef VOXELSPIRE_UI_SETTINGS_FILE_HPP
#define VOXELSPIRE_UI_SETTINGS_FILE_HPP

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "settings_model.hpp"

namespace voxelspire {

class SettingsFile {
public:
    static constexpr const char* KEY_SEPARATOR = " | ";
    static constexpr const char* UNBOUND       = "none";

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

    static std::uint32_t load(std::vector<SettingsTab>& tabs, const std::string& path) {
        std::ifstream in(path);
        if (!in) return Apply::Nothing;
        std::unordered_map<std::string, std::string> values;
        std::vector<std::string> order;
        std::string line;

        while (std::getline(in, line)) {
            const std::size_t eq = line.find('=');
            if (line.empty() || line[0] == '[' || line[0] == '#' || eq == std::string::npos) continue;
            const std::string key = trim(line.substr(0, eq));
            if (values.emplace(key, trim(line.substr(eq + 1))).second) order.push_back(key);
        }

        std::uint32_t applied = Apply::Nothing;

        for (SettingsTab& tab : tabs)
            for (SettingControl& c : tab.controls) {
                if (!c.persist) continue;
                auto it = values.find(c.key);
                if (it == values.end()) continue;
                if (assign(c, it->second)) applied |= c.apply;
            }

        return applied;
    }

private:
    static std::string trim(const std::string& s) {
        const std::size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return std::string();
        const std::size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    static std::string value_of(const SettingControl& c) {
        switch (c.kind) {
            case ControlKind::Toggle: return c.get_bool() ? "true" : "false";
            case ControlKind::Number: {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.10g", c.get_number());
                return buf;
            }
            case ControlKind::Choice: {
                const int i = c.get_choice();
                return i >= 0 && static_cast<std::size_t>(i) < c.options.size() ? c.options[static_cast<std::size_t>(i)] : std::string();
            }
            case ControlKind::Color: {
                const Color v = c.get_color();
                return std::to_string(v.red()) + ", " + std::to_string(v.green()) + ", " + std::to_string(v.blue()) + ", " + std::to_string(v.alpha());
            }
            case ControlKind::Binding: {
                if (!c.bindings) return UNBOUND;
                std::string out;
                for (const std::string& k : c.bindings->keys(c.action)) out += (out.empty() ? "" : KEY_SEPARATOR) + k;
                return out.empty() ? std::string(UNBOUND) : out;
            }
            default: return std::string();
        }
    }

    static bool assign(SettingControl& c, const std::string& text) {
        switch (c.kind) {
            case ControlKind::Toggle:
                if (text != "true" && text != "false") return false;
                c.set_bool(text == "true");
                return true;
            case ControlKind::Number: {
                char* end = nullptr;
                const double v = std::strtod(text.c_str(), &end);
                if (end == text.c_str()) return false;
                c.set_number(v);
                return true;
            }
            case ControlKind::Choice:
                for (std::size_t i = 0; i < c.options.size(); ++i) {
                    if (c.options[i] != text) continue;
                    c.set_choice(static_cast<int>(i));
                    return true;
                }
                return false;
            case ControlKind::Color: {
                int ch[4] = { 0, 0, 0, 255 };
                const int n = std::sscanf(text.c_str(), "%d , %d , %d , %d", &ch[0], &ch[1], &ch[2], &ch[3]);
                if (n < 3) return false;
                auto b = [](int v) { return static_cast<std::uint8_t>(vclamp(v, 0, 255)); };
                c.set_color(Color(b(ch[0]), b(ch[1]), b(ch[2]), b(ch[3])));
                return true;
            }
            case ControlKind::Binding: {
                if (!c.bindings) return false;
                InputBindings::Keys keys;
                
                if (text != UNBOUND) {
                    std::size_t start = 0;
                    const std::string sep = KEY_SEPARATOR;

                    while (start <= text.size()) {
                        const std::size_t at = text.find(sep, start);
                        const std::string key = trim(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
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
};

} // namespace voxelspire

#endif // VOXELSPIRE_UI_SETTINGS_FILE_HPP