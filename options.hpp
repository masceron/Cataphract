#pragma once

#include <cstdint>

#define MAX_PLY 127

struct Options
{
    static inline uintptr_t hash{64};
    static inline int threads{1};
    static inline int move_overhead{50};
    static inline bool verbose{false};
};

enum Values: int
{
    draw = 0,
    infinity = 32000,
    negative_infinity = -infinity,
    mate_value = 31999,
    mate_in_max_ply = mate_value - MAX_PLY,
    mated_in_max_ply = -mate_in_max_ply,
    score_none = -32500
};