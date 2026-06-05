#pragma once

#include <thread>

namespace UCI
{
    static std::thread search_thread;

    void bench(std::string_view args);
    void process();
}
