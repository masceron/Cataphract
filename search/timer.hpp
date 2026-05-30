#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

struct Timer
{
    static inline std::condition_variable timer_cv;
    static inline std::mutex timer_lock;
    static inline std::thread timer_thread;
    static inline std::chrono::time_point<std::chrono::steady_clock> begin_time;

    static inline std::atomic<bool> is_search_cancelled;
    static inline std::atomic<bool> running{false};

    static uint64_t elapsed()
    {
        const auto count = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - begin_time).count();
        return count;
    }

    static void await(const uint32_t time)
    {
        std::unique_lock lock(timer_lock);

        timer_cv.wait_for(lock, std::chrono::milliseconds(time), [] {
            return is_search_cancelled.load();
        });

        is_search_cancelled = true;
        running = false;
    }

    static void start(const uint32_t time)
    {
        if (running) return;
        is_search_cancelled = false;
        running = true;
        begin_time = std::chrono::steady_clock::now();
        timer_thread = std::thread(await, time);
    }

    static void stop()
    {
        is_search_cancelled = true;
        timer_cv.notify_all();
    }
};
