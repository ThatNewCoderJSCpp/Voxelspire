#include "input/input_bindings.hpp"

namespace voxelspire {

const std::vector<ActionInfo>& all_actions() {
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
        { Action::Inventory,          "inventory",            "Inventory",             "Open or close your inventory, status and the list of every item." },
        { Action::Use,                "use",                  "Use / drink",           "Hold while looking at water to drink." },
        { Action::Hotbar1,            "hotbar_1",             "Hotbar slot 1",         "Select the first hotbar slot." },
        { Action::Hotbar2,            "hotbar_2",             "Hotbar slot 2",         "Select hotbar slot 2." },
        { Action::Hotbar3,            "hotbar_3",             "Hotbar slot 3",         "Select hotbar slot 3." },
        { Action::Hotbar4,            "hotbar_4",             "Hotbar slot 4",         "Select hotbar slot 4." },
        { Action::Hotbar5,            "hotbar_5",             "Hotbar slot 5",         "Select hotbar slot 5." },
        { Action::Hotbar6,            "hotbar_6",             "Hotbar slot 6",         "Select hotbar slot 6." },
        { Action::Hotbar7,            "hotbar_7",             "Hotbar slot 7",         "Select hotbar slot 7." },
        { Action::Hotbar8,            "hotbar_8",             "Hotbar slot 8",         "Select hotbar slot 8." },
        { Action::Hotbar9,            "hotbar_9",             "Hotbar slot 9",         "Select hotbar slot 9." },
        { Action::Hotbar10,           "hotbar_10",            "Hotbar slot 10",        "Select the last hotbar slot, on the far right." },
        { Action::OpenMenu,           "open_menu",            "Settings menu",         "Open or close this menu. Also releases the mouse." },
        { Action::CycleCamera,        "cycle_camera",         "Cycle camera",          "Switch between first person and the third person views." },
        { Action::ToggleHud,          "toggle_hud",           "Toggle HUD",            "Show or hide the debug panel." },
        { Action::RenderDistanceUp,   "render_distance_up",   "Render distance +",     "Increase the render distance by one step." },
        { Action::RenderDistanceDown, "render_distance_down", "Render distance -",     "Decrease the render distance by one step." },
        { Action::CycleLighting,      "cycle_lighting",       "Next lighting preset",  "Cycle through the lighting presets." },
    };
    return actions;
}

InputBindings InputBindings::defaults() {
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
    b.bind(Action::Inventory,          { "e", "E" });
    b.bind(Action::Use,                { "r", "R" });
    b.bind(Action::Hotbar1,            { "1" });
    b.bind(Action::Hotbar2,            { "2" });
    b.bind(Action::Hotbar3,            { "3" });
    b.bind(Action::Hotbar4,            { "4" });
    b.bind(Action::Hotbar5,            { "5" });
    b.bind(Action::Hotbar6,            { "6" });
    b.bind(Action::Hotbar7,            { "7" });
    b.bind(Action::Hotbar8,            { "8" });
    b.bind(Action::Hotbar9,            { "9" });
    b.bind(Action::Hotbar10,           { "0" });
    return b;
}

void InputBindings::add_key(Action a, const std::string& key) {
    Keys& keys = m_map[a];
    for (const std::string& k : variants(key)) if (std::find(keys.begin(), keys.end(), k) == keys.end()) keys.push_back(k);
}

std::string InputBindings::describe(Action a) const {
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

std::vector<std::string> InputBindings::variants(const std::string& key) {
    if (key.size() != 1 || !std::isalpha(static_cast<unsigned char>(key[0]))) return { key };
    const char c = key[0];
    return { std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(c)))), std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) };
}

std::string InputBindings::display_name(const std::string& key) {
    if (key.size() == 1) return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(key[0]))));
    return key;
}

bool InputBindings::just_pressed(const fizmo::windows::InputManager& in, Action a) const {
    for (const auto& k : keys(a)) if (!k.empty() && in.is_key_just_pressed(k)) return true;
    return false;
}

} // namespace voxelspire
