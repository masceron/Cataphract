#pragma once

#include <cstdint>

struct Options
{
    static inline uintptr_t hash{64};
    static inline int threads{1};
    static inline int move_overhead{50};
    static inline bool show_currmove{false};
};