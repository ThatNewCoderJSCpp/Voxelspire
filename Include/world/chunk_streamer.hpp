#ifndef VOXELSPIRE_WORLD_CHUNK_STREAMER_HPP
#define VOXELSPIRE_WORLD_CHUNK_STREAMER_HPP

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../core/job_system.hpp"
#include "world_save.hpp"
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

    void request(std::vector<ColumnPos> wanted, const vector3d& center, double keep_distance);

    void update(int max_jobs);

    void set_archive(const ChunkArchive* archive) noexcept { m_archive = archive; }

    void load_now(const std::vector<ColumnPos>& columns);

    void note_changes(const std::vector<ChunkPos>& changed);

    bool save(const ChunkArchive& archive);

    ChunkOverrides collect_overrides(int x0, int y0, int x1, int y1) const;

    bool wanted(const ColumnPos& c) const noexcept { return m_wanted_set.count(c) != 0; }

    StreamingStats stats() const;

private:
    void submit(const ColumnPos& c);

    void on_generated(const ColumnPos& c, std::vector<std::unique_ptr<Chunk>> chunks);

    static std::vector<std::unique_ptr<Chunk>> restore(const ChunkArchive* archive, const ColumnPos& c, std::vector<std::unique_ptr<Chunk>> generated);

    void insert(const ColumnPos& c, std::vector<std::unique_ptr<Chunk>> chunks);

    void unload_far();

    World&                  m_world;
    const WorldGenerator&   m_generator;
    JobSystem&              m_jobs;
    std::vector<ColumnPos>  m_wanted;
    std::unordered_set<ColumnPos, ColumnPosHash> m_wanted_set;
    std::unordered_set<ColumnPos, ColumnPosHash> m_in_flight;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> m_store;
    std::unordered_set<ChunkPos, ChunkPosHash> m_modified;
    std::unordered_set<ColumnPos, ColumnPosHash> m_dirty;
    const ChunkArchive*     m_archive = nullptr;
    vector3d                m_center{};
    double                  m_keep_distance = 0.0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_WORLD_CHUNK_STREAMER_HPP