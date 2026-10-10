#include "core/identifier.hpp"

namespace voxelspire {

const char* kind_name(Kind k) noexcept {
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

Identifier::Identifier(Kind kind, std::string_view owner, std::initializer_list<std::string_view> path) {
    std::vector<std::string> parts;
    parts.reserve(path.size());
    for (std::string_view p : path) parts.emplace_back(p);
    m_index = intern(kind, std::string(owner), std::move(parts));
}

Identifier Identifier::parse(Kind kind, std::string_view text) {
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

Identifier Identifier::find(Kind kind, std::string_view text) noexcept {
    Table& t = table();
    std::lock_guard<std::mutex> lock(t.mutex);
    auto it = t.by_key.find(key_of(kind, std::string(text)));
    Identifier id;
    if (it != t.by_key.end()) id.m_index = it->second;
    return id;
}

bool Identifier::in_category(std::string_view category) const {
    const auto& p = entry().path;
    for (std::size_t i = 0; i + 1 < p.size(); ++i) if (p[i] == category) return true;
    return false;
}

bool Identifier::valid_part(const std::string& s) noexcept {
    if (s.empty()) return false;

    for (char ch : s) {
        const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.';
        if (!ok) return false;
    }

    return true;
}

auto Identifier::intern(Kind kind, std::string owner, std::vector<std::string> path) -> Index {
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

} // namespace voxelspire
