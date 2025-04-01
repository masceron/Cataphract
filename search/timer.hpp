#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

inline volatile bool is_search_cancelled = false;

inline std::condition_variable cv;
inline std::mutex timer_lock;

class Timer
{
public:
    static std::thread timer_thread;
    static volatile bool running;
    static void await()
    {
        std::unique_lock lock(timer_lock);
        cv.wait_for(lock, std::chrono::milliseconds(9500));
        running = false;
        is_search_cancelled = true;
    }
    static void start()
    {
        if (running) return;
        is_search_cancelled = false;
        running = true;
        timer_thread = std::thread(await);
    }
    static void stop()
    {
        cv.notify_all();
    }
};

volatile bool Timer::running = false;
std::thread Timer::timer_thread;