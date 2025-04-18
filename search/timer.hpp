#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

inline volatile bool is_search_cancelled = false;

inline std::condition_variable cv;
inline std::mutex timer_lock;

struct Timer
{
    static std::thread timer_thread;
    static volatile bool running;
    static std::chrono::time_point<std::chrono::high_resolution_clock> begin_time;
    static uint64_t elapsed()
    {
        const auto count = std::chrono::duration_cast<std::chrono::microseconds>((std::chrono::high_resolution_clock::now() - begin_time)).count();
        return count;
    }
    static void await(const int time)
    {
        std::unique_lock lock(timer_lock);
        cv.wait_for(lock, std::chrono::milliseconds(time));
        running = false;
        is_search_cancelled = true;
    }
    static void start(const int time)
    {
        if (running) return;
        is_search_cancelled = false;
        running = true;
        begin_time = std::chrono::high_resolution_clock::now();
        timer_thread = std::thread(await, time);
    }
    static void stop()
    {
        cv.notify_all();
    }
};

volatile bool Timer::running = false;
std::thread Timer::timer_thread;
std::chrono::time_point<std::chrono::high_resolution_clock> Timer::begin_time;