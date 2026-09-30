#ifndef VOXELSPIRE_INPUT_BINDINGS_HPP
#define VOXELSPIRE_INPUT_BINDINGS_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include "../fizmo.hpp"

namespace voxelspire {

enum class Action {
    MoveForward = 0, MoveBack, MoveLeft, MoveRight,
    Jump, Sprint, Crawl, Crouch,
    CycleCamera, ReleaseMouse, Respawn, ToggleHud,
    RenderDistanceUp, RenderDistanceDown,
    CycleLighting, ToggleHandLight, TimeForward, TimeBackward, ToggleDayCycle,
    Swim, CycleHudCorner, HudLarger, HudSmaller, Alt
};

struct ActionHash { std::size_t operator()(Action a) const noexcept { return static_cast<std::size_t>(a); } };

class InputBindings {
public:
    using Keys = std::vector<std::string>;

    static InputBindings defaults() {
        InputBindings b;
        b.bind(Action::MoveForward,  { "w", "W" });
        b.bind(Action::MoveBack,     { "s", "S" });
        b.bind(Action::MoveLeft,     { "a", "A" });
        b.bind(Action::MoveRight,    { "d", "D" });
        b.bind(Action::Jump,         { "Space" });
        b.bind(Action::Sprint,       { "LeftShift" });
        b.bind(Action::Crouch,       { "LeftControl" });
        b.bind(Action::Crawl,        { "z", "Z" });
        b.bind(Action::CycleCamera,  { "F5" });
        b.bind(Action::ReleaseMouse, { "Escape" });
        b.bind(Action::Respawn,      { "r", "R" });
        b.bind(Action::ToggleHud,    { "F3" });
        b.bind(Action::RenderDistanceUp,   { "=", "+" });
        b.bind(Action::RenderDistanceDown, { "-", "_" });
        b.bind(Action::CycleLighting,   { "F7" });
        b.bind(Action::ToggleHandLight, { "l", "L" });
        b.bind(Action::TimeForward,     { "]", "}" });
        b.bind(Action::TimeBackward,    { "[", "{" });
        b.bind(Action::ToggleDayCycle,  { "F8" });
        b.bind(Action::Swim,            { "LeftShift" });
        b.bind(Action::CycleHudCorner,  { "F4" });
        b.bind(Action::Alt,             { "LeftAlt" });
        b.bind(Action::HudLarger,       { "F10" });
        b.bind(Action::HudSmaller,      { "F9" });
        return b;
    }

    void bind(Action a, Keys keys) { m_map[a] = std::move(keys); }
    void add_key(Action a, const std::string& key) { m_map[a].push_back(key); }
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