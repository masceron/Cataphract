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

    static uint64_t elapsed();
    static void await(uint32_t time);
    static void start(uint32_t time);
    static void stop();
};
