#include "ui/settings_file.hpp"

namespace voxelspire {

bool SettingsFile::save(const std::vector<SettingsTab>& tabs, const std::string& path) {
    const std::filesystem::path target(path);
    const std::filesystem::path temp = target.string() + TEMP_SUFFIX;
    std::error_code ec;
    if (target.has_parent_path()) std::filesystem::create_directories(target.parent_path(), ec);
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) return false;
        write(tabs, out);
        if (!out) return false;
    }

    std::filesystem::rename(temp, target, ec);
    return !ec;
}

void SettingsFile::write(const std::vector<SettingsTab>& tabs, std::ostream& out) {
    for (const SettingsTab& tab : tabs) {
        out << "[" << tab.name << "]\n";

        for (const SettingControl& c : tab.controls) {
            if (!c.persist || !c.interactive() || c.kind == ControlKind::Button) continue;
            out << c.key << " = " << value_of(c) << "\n";
        }
            
        out << "\n";
    }
}

auto SettingsFile::read(std::istream& in) -> Values {
    Values values;
    std::string line;

    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (line.empty() || line[0] == '[' || line[0] == '#' || eq == std::string::npos) continue;
        values[trimmed(line.substr(0, eq))] = trimmed(line.substr(eq + 1));
    }

    return values;
}

SettingsLoad SettingsFile::load(std::vector<SettingsTab>& tabs, const std::string& path) {
    std::ifstream in(path);
    if (!in) in.open(std::filesystem::path(path).filename());
    if (!in) return {};
    SettingsLoad result = apply(tabs, read(in));
    result.found = true;
    return result;
}

SettingsLoad SettingsFile::apply(std::vector<SettingsTab>& tabs, const Values& values) {
    SettingsLoad result;
    result.found = true;

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

std::string SettingsFile::value_of(const SettingControl& c) {
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
        case ControlKind::Text: return c.get_text();
        case ControlKind::Binding: {
            if (!c.bindings) return UNBOUND;
            std::string out;
            for (const std::string& k : c.bindings->keys(c.action)) out += (out.empty() ? "" : KEY_SEPARATOR) + k;
            return out.empty() ? std::string(UNBOUND) : out;
        }
        default: return std::string();
    }
}

bool SettingsFile::assign(SettingControl& c, const std::string& text, std::string& problem) {
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
        case ControlKind::Text: {
            if (c.max_length > 0 && text.size() > c.max_length) { problem = "is longer than " + std::to_string(c.max_length) + " characters."; return false; }
            c.set_text(text);
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

bool SettingsFile::assign_color(SettingControl& c, const std::string& text, std::string& problem) {
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

} // namespace voxelspire
