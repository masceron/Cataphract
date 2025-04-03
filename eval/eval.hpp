#pragma once
#include <cstdint>

constexpr int32_t white_negamax = 1, black_negamax = -1;

constexpr int32_t pawn_weight = 126;
constexpr int32_t knight_weight = 781;
constexpr int32_t bishop_weight = 825;
constexpr int32_t rook_weight = 1276;
constexpr int32_t queen_weight = 2538;

inline int32_t material(const Position& pos)
{
    return pawn_weight * (std::popcount(pos.boards[P]) - std::popcount(pos.boards[p]))
        + knight_weight * (std::popcount(pos.boards[N]) - std::popcount(pos.boards[n]))
        + bishop_weight * (std::popcount(pos.boards[B]) - std::popcount(pos.boards[b]))
        + rook_weight * (std::popcount(pos.boards[R]) - std::popcount(pos.boards[r]))
        + queen_weight * (std::popcount(pos.boards[Q]) - std::popcount(pos.boards[q]));
}

inline int32_t eval(const Position& pos)
{
    int32_t score = 0;
    const int32_t color = pos.side_to_move == white ? 1 : -1;

    score = material(pos) * color;
    return score;
}