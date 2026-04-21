#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

struct Timer
{
    static inline std::condition_variable timer_cv;
    static inline std::mutex timer_lock;
    static inline std::thread timer_thread;
    static inline volatile bool running{false};
    static inline std::chrono::time_point<std::chrono::high_resolution_clock> begin_time;
    static inline volatile bool is_search_cancelled;

    static uint64_t elapsed()
    {
        const auto count = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - begin_time).count();
        return count;
    }

    static void await(const uint32_t time)
    {
        std::unique_lock lock(timer_lock);
        timer_cv.wait_for(lock, std::chrono::milliseconds(time));
        running = false;
        is_search_cancelled = true;
    }

    static void start(const uint32_t time)
    {
        if (running) return;
        is_search_cancelled = false;
        running = true;
        begin_time = std::chrono::high_resolution_clock::now();
        timer_thread = std::thread(await, time);
    }

    static void stop()
    {
        timer_cv.notify_all();
    }
};
