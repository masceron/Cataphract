#pragma once

#include <cstdint>

struct Options
{
    uintptr_t hash = 64;
    int move_overhead = 50;
    bool show_currmove = false;
};

inline Options options;