#include "world/world_save.hpp"

namespace voxelspire {

void WorldStateFormat::write(std::ostream& out, const WorldState& s) {
    if (!s.has_player && !s.has_time && !s.has_weather && !s.has_vitals && !s.has_inventory) return;
    char buf[BUFFER];

    auto line = [&](const char* key, double v) {
        std::snprintf(buf, sizeof(buf), "%s = %.17g\n", key, v);
        out << buf;
    };

    out << SECTION << "\n";

    if (s.has_player) {
        line(PLAYER_X, s.position.x);
        line(PLAYER_Y, s.position.y);
        line(PLAYER_Z, s.position.z);
        line(PLAYER_YAW, s.yaw);
        line(PLAYER_PITCH, s.pitch);
    }

    if (s.has_time) {
        line(CLOCK_TIME, s.time);
        out << CLOCK_DAY << " = " << s.day << "\n";
    }

    if (s.has_weather) {
        line(WEATHER_KIND, s.weather_kind);
        line(WEATHER_NOW, s.weather_intensity);
        line(WEATHER_GOAL, s.weather_target);
        line(WEATHER_LEFT, s.weather_remaining);
        line(WEATHER_HAIL, s.weather_hail ? 1.0 : 0.0);
    }

    if (s.has_vitals) {
        line(HEALTH, s.health);
        line(HUNGER, s.hunger);
        line(THIRST, s.thirst);
        line(STAMINA, s.stamina);
        line(BREATH, s.breath);
        line(DEAD, s.dead ? 1.0 : 0.0);
        line(STOMACH_BULK, s.stomach_bulk);
        line(STOMACH_FOOD, s.stomach_energy);
        line(STOMACH_WET, s.stomach_water);
    }

    if (s.has_inventory) {
        out << SELECTED << " = " << s.selected_slot << "\n";
        for (const SavedStack& st : s.inventory) out << SLOT_PREFIX << st.slot << " = " << st.item << " " << st.count << "\n";
        for (std::size_t i = 0; i < s.stack_sizes.size(); ++i) out << STACK_PREFIX << i << " = " << s.stack_sizes[i].item << " " << s.stack_sizes[i].count << "\n";
    }

    out << "\n";
}

WorldState WorldStateFormat::read(const Values& values) {
    WorldState s;

    auto number = [&](const char* key, double& out) {
        auto it = values.find(key);
        if (it == values.end()) return false;
        char* end = nullptr;
        const double v = std::strtod(it->second.c_str(), &end);
        if (end == it->second.c_str() || !std::isfinite(v)) return false;
        out = v;
        return true;
    };

    s.has_player = number(PLAYER_X, s.position.x) && number(PLAYER_Y, s.position.y) && number(PLAYER_Z, s.position.z);
    number(PLAYER_YAW, s.yaw);
    number(PLAYER_PITCH, s.pitch);
    double day = 0.0;
    s.has_time = number(CLOCK_TIME, s.time) && number(CLOCK_DAY, day);
    s.day = static_cast<std::int64_t>(day);
    double kind = 0.0, hail = 0.0;
    s.has_weather = number(WEATHER_KIND, kind) && number(WEATHER_NOW, s.weather_intensity) && number(WEATHER_GOAL, s.weather_target) && number(WEATHER_LEFT, s.weather_remaining);
    number(WEATHER_HAIL, hail);
    s.weather_kind = static_cast<int>(kind);
    s.weather_hail = hail != 0.0;
    double dead = 0.0;
    s.has_vitals = number(HEALTH, s.health) && number(HUNGER, s.hunger) && number(THIRST, s.thirst) && number(STAMINA, s.stamina) && number(BREATH, s.breath);
    number(DEAD, dead);
    number(STOMACH_BULK, s.stomach_bulk);
    number(STOMACH_FOOD, s.stomach_energy);
    number(STOMACH_WET, s.stomach_water);
    s.dead = dead != 0.0;
    read_inventory(values, s);
    return s;
}

bool WorldStateFormat::stack_value(const std::string& text, SavedStack& out) {
    const std::size_t space = text.rfind(' ');
    if (space == std::string::npos || space == 0) return false;
    char* end = nullptr;
    const long n = std::strtol(text.c_str() + space + 1, &end, DECIMAL);
    if (end == text.c_str() + space + 1) return false;
    out.item  = text.substr(0, space);
    out.count = static_cast<int>(n);
    return true;
}

void WorldStateFormat::read_inventory(const Values& values, WorldState& s) {
    const std::string slots = SLOT_PREFIX, stacks = STACK_PREFIX;

    for (const auto& kv : values) {
        const bool slot = kv.first.compare(0, slots.size(), slots) == 0;
        const bool stack = kv.first.compare(0, stacks.size(), stacks) == 0;
        if (!slot && !stack) continue;
        SavedStack st;
        if (!stack_value(kv.second, st)) continue;
        st.slot = static_cast<int>(std::strtol(kv.first.c_str() + (slot ? slots.size() : stacks.size()), nullptr, DECIMAL));
        (slot ? s.inventory : s.stack_sizes).push_back(st);
    }

    auto it = values.find(SELECTED);
    s.has_inventory = it != values.end();
    if (s.has_inventory) s.selected_slot = static_cast<int>(std::strtol(it->second.c_str(), nullptr, DECIMAL));
}

bool ChunkArchive::save_column(const ColumnPos& col, const std::vector<const Chunk*>& chunks) const {
    const std::filesystem::path path = file_for(col);
    std::error_code ec;

    if (chunks.empty()) {
        std::filesystem::remove(path, ec);
        return true;
    }

    std::filesystem::create_directories(m_dir, ec);
    if (ec) return false;
    std::vector<BlockId> palette;
    std::vector<std::uint16_t> remap(m_registry.size(), NOT_MAPPED);

    for (const Chunk* c : chunks)
        for (int i = 0; i < Chunk::VOLUME; ++i) {
            const BlockId id = c->data()[i];
            if (id < remap.size() && remap[id] == NOT_MAPPED) { remap[id] = static_cast<std::uint16_t>(palette.size()); palette.push_back(id); }
        }

    const std::filesystem::path temp = path.string() + TEMP_SUFFIX;
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        put32(out, MAGIC);
        put32(out, VERSION);
        put32(out, static_cast<std::uint32_t>(palette.size()));

        for (BlockId id : palette) {
            const std::string& name = m_registry.get(id).identifier().str();
            put16(out, static_cast<std::uint16_t>(name.size()));
            out.write(name.data(), static_cast<std::streamsize>(name.size()));
        }

        put32(out, static_cast<std::uint32_t>(chunks.size()));

        for (const Chunk* c : chunks) {
            put32(out, static_cast<std::uint32_t>(c->pos().z));
            const bool states = c->has_states();
            out.put(static_cast<char>(states ? HAS_STATES : 0));
            write_runs(out, *c, remap);
            if (states) out.write(reinterpret_cast<const char*>(c->states()), Chunk::VOLUME);
        }

        if (!out) return false;
    }

    std::filesystem::rename(temp, path, ec);
    return !ec;
}

std::vector<std::unique_ptr<Chunk>> ChunkArchive::load_column(const ColumnPos& col) const {
    std::vector<std::unique_ptr<Chunk>> out;
    std::ifstream in(file_for(col), std::ios::binary);
    if (!in) return out;
    if (get32(in) != MAGIC || get32(in) != VERSION) return out;
    const std::uint32_t count = get32(in);
    std::vector<BlockId> palette;
    palette.reserve(count);

    for (std::uint32_t i = 0; i < count && in; ++i) {
        std::string name(get16(in), '\0');
        in.read(&name[0], static_cast<std::streamsize>(name.size()));
        const auto id = m_registry.find(name);
        palette.push_back(id ? *id : AIR_ID);
    }

    const std::uint32_t chunks = get32(in);

    for (std::uint32_t k = 0; k < chunks && in; ++k) {
        const int z = static_cast<std::int32_t>(get32(in));
        const bool states = (static_cast<std::uint8_t>(in.get()) & HAS_STATES) != 0;
        auto chunk = std::make_unique<Chunk>(ChunkPos{ col.x, col.y, z });
        if (!read_runs(in, *chunk, palette)) return {};

        if (states) {
            std::vector<std::uint8_t> raw(Chunk::VOLUME);
            in.read(reinterpret_cast<char*>(raw.data()), Chunk::VOLUME);
            chunk->set_states(raw);
        }

        if (!in) return {};
        chunk->mark_modified();
        out.push_back(std::move(chunk));
    }

    return out;
}

void ChunkArchive::put16(std::ostream& out, std::uint16_t v) {
    out.put(static_cast<char>(v & BYTE_MASK));
    out.put(static_cast<char>((v >> BYTE_BITS) & BYTE_MASK));
}

std::uint16_t ChunkArchive::get16(std::istream& in) {
    const std::uint32_t a = static_cast<std::uint8_t>(in.get()), b = static_cast<std::uint8_t>(in.get());
    return static_cast<std::uint16_t>(a | (b << BYTE_BITS));
}

std::uint32_t ChunkArchive::get32(std::istream& in) {
    std::uint32_t v = 0;
    for (int i = 0; i < WORD_BYTES; ++i) v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(in.get())) << (i * BYTE_BITS);
    return v;
}

void ChunkArchive::write_runs(std::ostream& out, const Chunk& c, const std::vector<std::uint16_t>& remap) {
    const BlockId* data = c.data();
    int i = 0;

    while (i < Chunk::VOLUME) {
        const BlockId id = data[i];
        int run = 1;
        while (i + run < Chunk::VOLUME && data[i + run] == id && run < MAX_RUN) ++run;
        put16(out, static_cast<std::uint16_t>(run));
        put16(out, id < remap.size() ? remap[id] : 0);
        i += run;
    }
}

bool ChunkArchive::read_runs(std::istream& in, Chunk& c, const std::vector<BlockId>& palette) {
    std::vector<BlockId> blocks(Chunk::VOLUME, AIR_ID);
    int i = 0;

    while (i < Chunk::VOLUME && in) {
        const int run = get16(in);
        const std::uint16_t index = get16(in);
        if (run <= 0 || i + run > Chunk::VOLUME) return false;
        const BlockId id = index < palette.size() ? palette[index] : AIR_ID;
        std::fill(blocks.begin() + i, blocks.begin() + i + run, id);
        i += run;
    }

    if (i != Chunk::VOLUME) return false;
    c.set_blocks(blocks);
    return true;
}

} // namespace voxelspire
