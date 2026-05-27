#pragma once

#include <utility>

#include "arch.hpp"
#include "../board/bitboard.hpp"

inline uint8_t flip_color(const uint8_t piece)
{
    switch (piece)
    {
    case P:
    case N:
    case B:
    case R:
    case Q:
    case K:
        return piece + 6;
    case p:
    case n:
    case b:
    case r:
    case q:
    case k:
        return piece - 6;
    default:
        return piece;
    }
}

inline std::pair<int, int> input_index_of(const uint8_t p, const uint8_t sq,
                                                    const std::pair<bool, bool>& mirrors)
{
    return {p * 64 + (sq ^ 56 ^ (mirrors.first ? 7 : 0)), flip_color(p) * 64 + (sq ^ (mirrors.second ? 7 : 0))};
}

inline uint64_t horizontal_mirror(uint64_t board)
{
    static constexpr uint64_t k1 = 0x5555555555555555ull;
    static constexpr uint64_t k2 = 0x3333333333333333ull;
    static constexpr uint64_t k4 = 0x0f0f0f0f0f0f0f0full;
    board = ((board >> 1) & k1) | ((board & k1) << 1);
    board = ((board >> 2) & k2) | ((board & k2) << 2);
    board = ((board >> 4) & k4) | ((board & k4) << 4);
    return board;
}

inline std::pair<uint8_t, uint8_t> get_buckets(const uint64_t* boards)
{
    return {
        input_buckets_map[lsb(boards[K]) ^ 56], input_buckets_map[lsb(boards[k])]
    };
}