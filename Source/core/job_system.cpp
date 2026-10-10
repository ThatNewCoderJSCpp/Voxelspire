#include "core/job_system.hpp"

namespace voxelspire {

JobSystem::JobSystem(int workers) {
    if (workers <= 0) {
        const int hw = static_cast<int>(std::thread::hardware_concurrency());
        workers = hw - RESERVED_THREADS;
    }

    if (workers < MIN_WORKERS) workers = MIN_WORKERS;
    m_threads.reserve(static_cast<std::size_t>(workers));
    for (int i = 0; i < workers; ++i) m_threads.emplace_back([this] { run_worker(); });
}

void JobSystem::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_jobs_mutex);
        m_stopping = true;
        m_jobs.clear();
    }

    m_jobs_cv.notify_all();
    for (std::thread& t : m_threads) if (t.joinable()) t.join();
    m_threads.clear();
    std::lock_guard<std::mutex> lock(m_done_mutex);
    m_done.clear();
}

void JobSystem::submit(Task job) {
    {
        std::lock_guard<std::mutex> lock(m_jobs_mutex);
        if (m_stopping) return;
        m_jobs.push_back(std::move(job));
        ++m_in_flight;
    }

    m_jobs_cv.notify_one();
}

std::size_t JobSystem::run_completions(double budget_ms) {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double, std::milli>(budget_ms));
    std::size_t ran = 0;

    for (;;) {
        Task t;

        {
            std::lock_guard<std::mutex> lock(m_done_mutex);
            if (m_done.empty()) break;
            t = std::move(m_done.front());
            m_done.pop_front();
        }

        t();
        ++ran;
        if (clock::now() >= deadline) break;
    }

    return ran;
}

void JobSystem::wait_idle() {
    std::unique_lock<std::mutex> lock(m_jobs_mutex);
    m_idle_cv.wait(lock, [this] { return m_in_flight == 0; });
}

void JobSystem::run_worker() {
    for (;;) {
        Task job;

        {
            std::unique_lock<std::mutex> lock(m_jobs_mutex);
            m_jobs_cv.wait(lock, [this] { return m_stopping || !m_jobs.empty(); });
            if (m_stopping) return;
            job = std::move(m_jobs.front());
            m_jobs.pop_front();
        }

        job();

        {
            std::lock_guard<std::mutex> lock(m_jobs_mutex);
            --m_in_flight;
        }

        m_idle_cv.notify_all();
    }
}

} // namespace voxelspire
