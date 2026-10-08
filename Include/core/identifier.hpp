#ifndef VOXELSPIRE_CORE_IDENTIFIER_HPP
#define VOXELSPIRE_CORE_IDENTIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace voxelspire {

enum class Kind : std::uint8_t {
    None = 0,
    Block,
    Item,
    Entity,
    Biome,
    Particle,
    Attribute,
    Movement,
    Pose,
    Feature,
    Physics,
    Preset,
    Damage,
    Survival,
    Other
};

inline const char* kind_name(Kind k) noexcept {
    switch (k) {
        case Kind::None:      return "none";
        case Kind::Block:     return "block";
        case Kind::Item:      return "item";
        case Kind::Entity:    return "entity";
        case Kind::Biome:     return "biome";
        case Kind::Particle:  return "particle";
        case Kind::Attribute: return "attribute";
        case Kind::Movement:  return "movement";
        case Kind::Pose:      return "pose";
        case Kind::Feature:   return "feature";
        case Kind::Physics:   return "physics";
        case Kind::Preset:    return "preset";
        case Kind::Damage:    return "damage";
        case Kind::Survival:  return "survival";
        case Kind::Other:     return "other";
    }

    return "other";
}

class Identifier {
public:
    using Index = std::uint32_t;
    static constexpr Index NONE      = 0;
    static constexpr char  SEPARATOR = ':';

    constexpr Identifier() noexcept = default;

    Identifier(Kind kind, std::string_view owner, std::string_view name) : Identifier(kind, owner, { name }) {}

    Identifier(Kind kind, std::string_view owner, std::initializer_list<std::string_view> path) {
        std::vector<std::string> parts;
        parts.reserve(path.size());
        for (std::string_view p : path) parts.emplace_back(p);
        m_index = intern(kind, std::string(owner), std::move(parts));
    }

    Identifier(Kind kind, std::string_view owner, const std::vector<std::string>& path) : m_index(intern(kind, std::string(owner), path)) {}

    static Identifier parse(Kind kind, std::string_view text) {
        std::vector<std::string> parts;
        std::size_t start = 0;

        while (start <= text.size()) {
            const std::size_t end = text.find(SEPARATOR, start);
            parts.emplace_back(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
            if (end == std::string_view::npos) break;
            start = end + 1;
        }

        if (parts.size() < MIN_PARTS) throw std::invalid_argument("identifier needs an owner and a name: " + std::string(text));
        const std::string owner = parts.front();
        parts.erase(parts.begin());
        return Identifier(kind, owner, parts);
    }

    static std::optional<Identifier> try_parse(Kind kind, std::string_view text) noexcept {
        try { return parse(kind, text); } catch (...) { return std::nullopt; }
    }

    static Identifier find(Kind kind, std::string_view text) noexcept {
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        auto it = t.by_key.find(key_of(kind, std::string(text)));
        Identifier id;
        if (it != t.by_key.end()) id.m_index = it->second;
        return id;
    }

    Index index() const noexcept { return m_index; }
    bool  valid() const noexcept { return m_index != NONE; }
    explicit operator bool() const noexcept { return valid(); }

    Kind                            kind()       const { return entry().kind; }
    const std::string&              owner()      const { return entry().owner; }
    const std::vector<std::string>& path()       const { return entry().path; }
    const std::string&              name()       const { return entry().path.back(); }
    const std::string&              str()        const { return entry().text; }

    std::vector<std::string> categories() const {
        const auto& p = entry().path;
        return std::vector<std::string>(p.begin(), p.end() - 1);
    }

    bool in_category(std::string_view category) const {
        const auto& p = entry().path;
        for (std::size_t i = 0; i + 1 < p.size(); ++i) if (p[i] == category) return true;
        return false;
    }

    static std::size_t count() noexcept {
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        return t.entries.size();
    }

    constexpr bool operator==(const Identifier& o) const noexcept { return m_index == o.m_index; }
    constexpr bool operator!=(const Identifier& o) const noexcept { return m_index != o.m_index; }
    constexpr bool operator<(const Identifier& o) const noexcept { return m_index < o.m_index; }

private:
    static constexpr std::size_t MIN_PARTS = 2;

    struct Entry {
        Kind                     kind = Kind::None;
        std::string              owner;
        std::vector<std::string> path{ std::string() };
        std::string              text;
    };

    struct Table {
        std::mutex                             mutex;
        std::deque<Entry>                      entries{ Entry{} };
        std::unordered_map<std::string, Index> by_key;
    };

    static Table& table() {
        static Table t;
        return t;
    }

    const Entry& entry() const {
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        return t.entries[m_index];
    }

    static bool valid_part(const std::string& s) noexcept {
        if (s.empty()) return false;

        for (char ch : s) {
            const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.';
            if (!ok) return false;
        }

        return true;
    }

    static std::string key_of(Kind kind, const std::string& text) {
        return std::to_string(static_cast<int>(kind)) + "|" + text;
    }

    static Index intern(Kind kind, std::string owner, std::vector<std::string> path) {
        if (!valid_part(owner)) throw std::invalid_argument("identifier owner must be lowercase letters, digits, _ - or .: " + owner);
        if (path.empty()) throw std::invalid_argument("identifier needs a name: " + owner);

        std::string text = owner;

        for (const std::string& p : path) {
            if (!valid_part(p)) throw std::invalid_argument("identifier parts must be lowercase letters, digits, _ - or .: " + p);
            text += SEPARATOR;
            text += p;
        }

        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        const std::string key = key_of(kind, text);
        auto it = t.by_key.find(key);
        if (it != t.by_key.end()) return it->second;
        const Index index = static_cast<Index>(t.entries.size());
        t.entries.push_back(Entry{ kind, std::move(owner), std::move(path), std::move(text) });
        t.by_key.emplace(key, index);
        return index;
    }

    Index m_index = NONE;
};

inline constexpr const char* CORE_OWNER = "voxelspire";

inline Identifier core_id(Kind kind, std::string_view name) { return Identifier(kind, CORE_OWNER, name); }
inline Identifier core_id(Kind kind, std::initializer_list<std::string_view> path) { return Identifier(kind, CORE_OWNER, path); }

struct IdentifierHash {
    std::size_t operator()(const Identifier& id) const noexcept { return static_cast<std::size_t>(id.index()) * 0x9E3779B97F4A7C15ull; }
};

template <typename T>
class IdentifierTable {
public:
    T* find(const Identifier& id) noexcept {
        const std::size_t i = id.index();
        return i < m_slots.size() && m_slots[i] ? &*m_slots[i] : nullptr;
    }

    const T* find(const Identifier& id) const noexcept {
        const std::size_t i = id.index();
        return i < m_slots.size() && m_slots[i] ? &*m_slots[i] : nullptr;
    }

    template <typename... Args>
    T& emplace(const Identifier& id, Args&&... args) {
        const std::size_t i = id.index();
        if (i >= m_slots.size()) m_slots.resize(i + 1);
        if (!m_slots[i]) ++m_count;
        m_slots[i].emplace(std::forward<Args>(args)...);
        return *m_slots[i];
    }

    bool erase(const Identifier& id) noexcept {
        const std::size_t i = id.index();
        if (i >= m_slots.size() || !m_slots[i]) return false;
        m_slots[i].reset();
        --m_count;
        return true;
    }

    bool        contains(const Identifier& id) const noexcept { return find(id) != nullptr; }
    std::size_t size()                         const noexcept { return m_count; }

    template <typename Fn>
    void for_each(Fn&& fn) const {
        for (std::size_t i = 0; i < m_slots.size(); ++i)
            if (m_slots[i]) fn(*m_slots[i]);
    }

    template <typename Fn>
    void for_each(Fn&& fn) {
        for (std::size_t i = 0; i < m_slots.size(); ++i)
            if (m_slots[i]) fn(*m_slots[i]);
    }

private:
    std::vector<std::optional<T>> m_slots;
    std::size_t                   m_count = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_IDENTIFIER_HPP