#include "timer.hpp"

uint64_t Timer::elapsed()
{
    const auto count = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - begin_time).count();
    return count;
}

void Timer::await(const uint32_t time)
{
    std::unique_lock lock(timer_lock);

    timer_cv.wait_for(lock, std::chrono::milliseconds(time), []
    {
        return is_search_cancelled.load();
    });

    is_search_cancelled = true;
    running = false;
}

void Timer::start(const uint32_t time)
{
    if (running) return;
    is_search_cancelled = false;
    running = true;
    begin_time = std::chrono::steady_clock::now();
    timer_thread = std::thread(await, time);
}

void Timer::stop()
{
    is_search_cancelled = true;
    timer_cv.notify_all();
}
