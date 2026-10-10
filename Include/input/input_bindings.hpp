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
    CycleLighting, Swim, Alt,
    Inventory, Use,
    Hotbar1, Hotbar2, Hotbar3, Hotbar4, Hotbar5, Hotbar6, Hotbar7, Hotbar8, Hotbar9, Hotbar10
};

struct ActionInfo {
    Action      action;
    const char* key;
    const char* label;
    const char* description;
};

const std::vector<ActionInfo>& all_actions();

struct ActionHash { std::size_t operator()(Action a) const noexcept { return static_cast<std::size_t>(a); } };

class InputBindings {
public:
    using Keys = std::vector<std::string>;

    static InputBindings defaults();

    void bind(Action a, Keys keys) { m_map[a] = std::move(keys); }
    void unbind(Action a) { m_map[a].clear(); }

    void add_key(Action a, const std::string& key);

    void set_key(Action a, const std::string& key) {
        m_map[a].clear();
        add_key(a, key);
    }

    std::string describe(Action a) const;

    static std::vector<std::string> variants(const std::string& key);

    static std::string display_name(const std::string& key);
    const Keys& keys(Action a) const { static const Keys none; auto it = m_map.find(a); return it == m_map.end() ? none : it->second; }

    bool is_down(const fizmo::windows::InputManager& in, Action a) const {
        for (const auto& k : keys(a)) if (!k.empty() && in.is_key_down(k)) return true;
        return false;
    }

    bool just_pressed(const fizmo::windows::InputManager& in, Action a) const;

private:
    std::unordered_map<Action, Keys, ActionHash> m_map;
};

} // namespace voxelspire

#endif // VOXELSPIRE_INPUT_BINDINGS_HPP