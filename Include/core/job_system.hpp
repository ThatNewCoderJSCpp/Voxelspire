#ifndef VOXELSPIRE_CORE_JOB_SYSTEM_HPP
#define VOXELSPIRE_CORE_JOB_SYSTEM_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace voxelspire {

class Task {
public:
    Task() = default;

    template <typename Fn, typename = typename std::enable_if<!std::is_same<typename std::decay<Fn>::type, Task>::value>::type>
    Task(Fn&& fn) : m_impl(new Impl<typename std::decay<Fn>::type>(std::forward<Fn>(fn))) {}

    Task(Task&&) noexcept = default;
    Task& operator=(Task&&) noexcept = default;

    explicit operator bool() const noexcept { return m_impl != nullptr; }
    void operator()() { m_impl->call(); }

private:
    struct Base {
        virtual ~Base() = default;
        virtual void call() = 0;
    };

    template <typename Fn>
    struct Impl final : Base {
        explicit Impl(Fn&& f) : fn(std::move(f)) {}
        explicit Impl(const Fn& f) : fn(f) {}
        void call() override { fn(); }
        Fn fn;
    };

    std::unique_ptr<Base> m_impl;
};

class JobSystem {
public:
    static constexpr int MIN_WORKERS = 1;
    static constexpr int RESERVED_THREADS = 1;

    explicit JobSystem(int workers = 0) {
        if (workers <= 0) {
            const int hw = static_cast<int>(std::thread::hardware_concurrency());
            workers = hw - RESERVED_THREADS;
        }

        if (workers < MIN_WORKERS) workers = MIN_WORKERS;
        m_threads.reserve(static_cast<std::size_t>(workers));
        for (int i = 0; i < workers; ++i) m_threads.emplace_back([this] { run_worker(); });
    }

    ~JobSystem() { shutdown(); }

    void shutdown() {
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

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void submit(Task job) {
        {
            std::lock_guard<std::mutex> lock(m_jobs_mutex);
            if (m_stopping) return;
            m_jobs.push_back(std::move(job));
            ++m_in_flight;
        }

        m_jobs_cv.notify_one();
    }

    void post(Task completion) {
        std::lock_guard<std::mutex> lock(m_done_mutex);
        m_done.push_back(std::move(completion));
    }

    std::size_t run_completions(double budget_ms) {
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

    void wait_idle() {
        std::unique_lock<std::mutex> lock(m_jobs_mutex);
        m_idle_cv.wait(lock, [this] { return m_in_flight == 0; });
    }

    std::size_t pending() const {
        std::lock_guard<std::mutex> lock(m_jobs_mutex);
        return m_in_flight;
    }

    std::size_t completions_waiting() const {
        std::lock_guard<std::mutex> lock(m_done_mutex);
        return m_done.size();
    }

    int worker_count() const noexcept { return static_cast<int>(m_threads.size()); }

private:
    void run_worker() {
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

    mutable std::mutex       m_jobs_mutex;
    std::condition_variable  m_jobs_cv;
    std::condition_variable  m_idle_cv;
    std::deque<Task>         m_jobs;
    std::size_t              m_in_flight = 0;
    bool                     m_stopping  = false;

    mutable std::mutex       m_done_mutex;
    std::deque<Task>         m_done;

    std::vector<std::thread> m_threads;
};

} // namespace voxelspire

#endif // VOXELSPIRE_CORE_JOB_SYSTEM_HPP