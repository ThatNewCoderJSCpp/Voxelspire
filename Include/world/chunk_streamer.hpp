#ifndef VOXELSPIRE_WORLD_CHUNK_STREAMER_HPP
#define VOXELSPIRE_WORLD_CHUNK_STREAMER_HPP

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../core/job_system.hpp"
#include "world_generator.hpp"

namespace voxelspire {

struct StreamingStats {
    std::size_t loaded_columns  = 0;
    std::size_t wanted_columns  = 0;
    std::size_t jobs_in_flight  = 0;
    std::size_t stored_chunks   = 0;
    std::size_t modified_loaded = 0;
};

class ChunkStreamer {
public:
    using ChunkOverrides = std::unordered_map<ChunkPos, std::shared_ptr<const Chunk>, ChunkPosHash>;

    ChunkStreamer(World& world, const WorldGenerator& generator, JobSystem& jobs)
        : m_world(world), m_generator(generator), m_jobs(jobs) {}

    void request(std::vector<ColumnPos> wanted, const vector3d& center, double keep_distance) {
        m_wanted = std::move(wanted);
        m_wanted_set.clear();
        m_wanted_set.insert(m_wanted.begin(), m_wanted.end());
        m_center = center;
        m_keep_distance = keep_distance;
        unload_far();
    }

    void update(int max_jobs) {
        for (const ColumnPos& c : m_wanted) {
            if (static_cast<int>(m_in_flight.size()) >= max_jobs) break;
            if (m_world.column_loaded(c) || m_in_flight.count(c)) continue;
            submit(c);
        }
    }

    void load_now(const std::vector<ColumnPos>& columns) {
        for (const ColumnPos& c : columns) {
            if (m_world.column_loaded(c)) continue;
            insert(c, m_generator.generate_column(c, m_world.min_chunk_z(), m_world.max_chunk_z()));
        }
    }

    void note_changes(const std::vector<ChunkPos>& changed) {
        for (const ChunkPos& p : changed) {
            const Chunk* c = m_world.chunk_at(p);
            if (c && c->modified()) m_modified.insert(p);
        }
    }

    ChunkOverrides collect_overrides(int x0, int y0, int x1, int y1) const {
        ChunkOverrides out;
        const int cx0 = floor_div(x0, Chunk::SIZE), cx1 = floor_div(x1 - 1, Chunk::SIZE);
        const int cy0 = floor_div(y0, Chunk::SIZE), cy1 = floor_div(y1 - 1, Chunk::SIZE);
        auto inside = [&](const ChunkPos& p) { return p.x >= cx0 && p.x <= cx1 && p.y >= cy0 && p.y <= cy1; };

        for (const auto& kv : m_store)
            if (inside(kv.first)) out[kv.first] = std::make_shared<const Chunk>(*kv.second);

        for (const ChunkPos& p : m_modified) {
            if (!inside(p)) continue;
            if (const Chunk* c = m_world.chunk_at(p)) out[p] = std::make_shared<const Chunk>(*c);
        }

        return out;
    }

    bool wanted(const ColumnPos& c) const noexcept { return m_wanted_set.count(c) != 0; }

    StreamingStats stats() const {
        StreamingStats s;
        s.loaded_columns  = m_world.columns().size();
        s.wanted_columns  = m_wanted.size();
        s.jobs_in_flight  = m_in_flight.size();
        s.stored_chunks   = m_store.size();
        s.modified_loaded = m_modified.size();
        return s;
    }

private:
    void submit(const ColumnPos& c) {
        m_in_flight.insert(c);
        const WorldGenerator* gen = &m_generator;
        const int zmin = m_world.min_chunk_z(), zmax = m_world.max_chunk_z();
        JobSystem* jobs = &m_jobs;

        m_jobs.submit([this, gen, jobs, c, zmin, zmax] {
            auto chunks = std::make_shared<std::vector<std::unique_ptr<Chunk>>>(gen->generate_column(c, zmin, zmax));
            jobs->post([this, c, chunks] { on_generated(c, std::move(*chunks)); });
        });
    }

    void on_generated(const ColumnPos& c, std::vector<std::unique_ptr<Chunk>> chunks) {
        m_in_flight.erase(c);
        if (!m_wanted_set.count(c) || m_world.column_loaded(c)) return;
        insert(c, std::move(chunks));
    }

    void insert(const ColumnPos& c, std::vector<std::unique_ptr<Chunk>> chunks) {
        std::vector<std::unique_ptr<Chunk>> final_chunks;

        for (auto& chunk : chunks) {
            if (!chunk || m_store.count(chunk->pos())) continue;
            final_chunks.push_back(std::move(chunk));
        }

        for (auto it = m_store.begin(); it != m_store.end();) {
            if (it->first.x == c.x && it->first.y == c.y) {
                m_modified.insert(it->first);
                final_chunks.push_back(std::move(it->second));
                it = m_store.erase(it);
            } else {
                ++it;
            }
        }

        m_world.insert_column(c, std::move(final_chunks));
    }

    void unload_far() {
        std::vector<ColumnPos> drop;

        for (const auto& kv : m_world.columns()) {
            const ColumnPos& c = kv.first;
            if (m_wanted_set.count(c)) continue;
            const double dx = (c.x + 0.5) * Chunk::SIZE - m_center.x, dy = (c.y + 0.5) * Chunk::SIZE - m_center.y;
            if (dx * dx + dy * dy <= m_keep_distance * m_keep_distance) continue;
            drop.push_back(c);
        }

        for (const ColumnPos& c : drop) {
            for (auto& chunk : m_world.remove_column(c)) {
                const ChunkPos p = chunk->pos();
                m_modified.erase(p);
                if (chunk->modified()) m_store[p] = std::move(chunk);
            }
        }
    }

    World&                  m_world;
    const WorldGenerator&   m_generator;
    JobSystem&              m_jobs;
    std::vector<ColumnPos>  m_wanted;
    std::unordered_set<ColumnPos, ColumnPosHash> m_wanted_set;
    std::unordered_set<ColumnPos, ColumnPosHash> m_in_flight;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> m_store;
    std::unordered_set<ChunkPos, ChunkPosHash> m_modified;
    vector3d                m_center{};
    double                  m_keep_distance = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CHUNK_STREAMER_HPP