#ifndef VOXELSPIRE_CORE_IDENTIFIER_HPP
#define VOXELSPIRE_CORE_IDENTIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace voxelspire {

class Identifier {
public:
    using Index = std::uint32_t;
    static constexpr Index NONE = 0;

    constexpr Identifier() noexcept = default;
    Identifier(const char* name) : m_index(intern(name)) {}
    Identifier(const std::string& name) : m_index(intern(name)) {}

    static Identifier find(const std::string& name) noexcept {
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        auto it = t.by_name.find(name);
        Identifier id;
        if (it != t.by_name.end()) id.m_index = it->second;
        return id;
    }

    Index index() const noexcept { return m_index; }
    bool  valid() const noexcept { return m_index != NONE; }
    explicit operator bool() const noexcept { return valid(); }

    const std::string& str() const {
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        return t.names[m_index];
    }

    static std::size_t count() noexcept {
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        return t.names.size();
    }

    constexpr bool operator==(const Identifier& o) const noexcept { return m_index == o.m_index; }
    constexpr bool operator!=(const Identifier& o) const noexcept { return m_index != o.m_index; }
    constexpr bool operator<(const Identifier& o) const noexcept { return m_index < o.m_index; }

private:
    struct Table {
        std::mutex                             mutex;
        std::deque<std::string>                names{ std::string() };
        std::unordered_map<std::string, Index> by_name;
    };

    static Table& table() {
        static Table t;
        return t;
    }

    static Index intern(const std::string& name) {
        if (name.empty()) return NONE;
        Table& t = table();
        std::lock_guard<std::mutex> lock(t.mutex);
        auto it = t.by_name.find(name);
        if (it != t.by_name.end()) return it->second;
        const Index index = static_cast<Index>(t.names.size());
        t.names.push_back(name);
        t.by_name.emplace(name, index);
        return index;
    }

    Index m_index = NONE;
};

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