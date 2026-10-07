#ifndef VOXELSPIRE_INPUT_BINDINGS_HPP
#define VOXELSPIRE_INPUT_BINDINGS_HPP

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>
#include "../fizmo.hpp"

namespace voxelspire {

enum class Action {
    MoveForward = 0, MoveBack, MoveLeft, MoveRight,
    Jump, Sprint, Crawl, Crouch,
    CycleCamera, OpenMenu, ToggleHud,
    RenderDistanceUp, RenderDistanceDown,
    CycleLighting, Swim, Alt
};

struct ActionInfo {
    Action      action;
    const char* key;
    const char* label;
    const char* description;
};

inline const std::vector<ActionInfo>& all_actions() {
    static const std::vector<ActionInfo> actions = {
        { Action::MoveForward,        "move_forward",         "Move forward",          "Walk forward." },
        { Action::MoveBack,           "move_back",            "Move back",             "Walk backward." },
        { Action::MoveLeft,           "move_left",            "Move left",             "Strafe left." },
        { Action::MoveRight,          "move_right",           "Move right",            "Strafe right." },
        { Action::Jump,               "jump",                 "Jump",                  "Jump, rise while swimming, or leap out of the water." },
        { Action::Sprint,             "sprint",               "Sprint",                "Run faster while moving forward." },
        { Action::Crouch,             "crouch",               "Crouch",                "Crouch; also sinks while swimming." },
        { Action::Crawl,              "crawl",                "Crawl",                 "Lie prone and crawl through one-block gaps." },
        { Action::Swim,               "swim",                 "Swim stroke",           "Swim in the direction you look while in water." },
        { Action::Alt,                "alt",                  "Alternate speed",       "Hold to switch the current movement to its alternate speed." },
        { Action::OpenMenu,           "open_menu",            "Settings menu",         "Open or close this menu. Also releases the mouse." },
        { Action::CycleCamera,        "cycle_camera",         "Cycle camera",          "Switch between first person and the third person views." },
        { Action::ToggleHud,          "toggle_hud",           "Toggle HUD",            "Show or hide the debug panel." },
        { Action::RenderDistanceUp,   "render_distance_up",   "Render distance +",     "Increase the render distance by one step." },
        { Action::RenderDistanceDown, "render_distance_down", "Render distance -",     "Decrease the render distance by one step." },
        { Action::CycleLighting,      "cycle_lighting",       "Next lighting preset",  "Cycle through the lighting presets." },
    };
    return actions;
}

struct ActionHash { std::size_t operator()(Action a) const noexcept { return static_cast<std::size_t>(a); } };

class InputBindings {
public:
    using Keys = std::vector<std::string>;

    static InputBindings defaults() {
        InputBindings b;
        b.bind(Action::MoveForward,        { "w", "W" });
        b.bind(Action::MoveBack,           { "s", "S" });
        b.bind(Action::MoveLeft,           { "a", "A" });
        b.bind(Action::MoveRight,          { "d", "D" });
        b.bind(Action::Jump,               { "Space" });
        b.bind(Action::Sprint,             { "LeftShift" });
        b.bind(Action::Crouch,             { "LeftControl" });
        b.bind(Action::Crawl,              { "z", "Z" });
        b.bind(Action::CycleCamera,        { "F5" });
        b.bind(Action::OpenMenu,           { "Escape" });
        b.bind(Action::ToggleHud,          { "F3" });
        b.bind(Action::CycleLighting,      { "F7" });
        b.bind(Action::Swim,               { "LeftShift" });
        b.bind(Action::Alt,                { "LeftAlt" });
        return b;
    }

    void bind(Action a, Keys keys) { m_map[a] = std::move(keys); }
    void unbind(Action a) { m_map[a].clear(); }

    void add_key(Action a, const std::string& key) {
        Keys& keys = m_map[a];
        for (const std::string& k : variants(key)) if (std::find(keys.begin(), keys.end(), k) == keys.end()) keys.push_back(k);
    }

    void set_key(Action a, const std::string& key) {
        m_map[a].clear();
        add_key(a, key);
    }

    std::string describe(Action a) const {
        std::string out;
        std::vector<std::string> seen;

        for (const std::string& k : keys(a)) {
            const std::string shown = display_name(k);
            if (std::find(seen.begin(), seen.end(), shown) != seen.end()) continue;
            seen.push_back(shown);
            if (!out.empty()) out += ", ";
            out += shown;
        }

        return out.empty() ? std::string("Unbound") : out;
    }

    static std::vector<std::string> variants(const std::string& key) {
        if (key.size() != 1 || !std::isalpha(static_cast<unsigned char>(key[0]))) return { key };
        const char c = key[0];
        return { std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(c)))), std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) };
    }

    static std::string display_name(const std::string& key) {
        if (key.size() == 1) return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(key[0]))));
        return key;
    }
    const Keys& keys(Action a) const { static const Keys none; auto it = m_map.find(a); return it == m_map.end() ? none : it->second; }

    bool is_down(const fizmo::windows::InputManager& in, Action a) const {
        for (const auto& k : keys(a)) if (!k.empty() && in.is_key_down(k)) return true;
        return false;
    }

    bool just_pressed(const fizmo::windows::InputManager& in, Action a) const {
        for (const auto& k : keys(a)) if (!k.empty() && in.is_key_just_pressed(k)) return true;
        return false;
    }

private:
    std::unordered_map<Action, Keys, ActionHash> m_map;
};

} // namespace voxelspire

#endif // VOXELSPIRE_INPUT_BINDINGS_HPP