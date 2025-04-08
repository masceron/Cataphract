#pragma once
#include <cstdint>

constexpr int16_t white_negamax = 1, black_negamax = -1;

constexpr int16_t pawn_weight = 126;
constexpr int16_t knight_weight = 781;
constexpr int16_t bishop_weight = 825;
constexpr int16_t rook_weight = 1276;
constexpr int16_t queen_weight = 2538;

inline int16_t material(const Position& pos)
{
    return pawn_weight * (std::popcount(pos.boards[P]) - std::popcount(pos.boards[p]))
        + knight_weight * (std::popcount(pos.boards[N]) - std::popcount(pos.boards[n]))
        + bishop_weight * (std::popcount(pos.boards[B]) - std::popcount(pos.boards[b]))
        + rook_weight * (std::popcount(pos.boards[R]) - std::popcount(pos.boards[r]))
        + queen_weight * (std::popcount(pos.boards[Q]) - std::popcount(pos.boards[q]));
}

inline int16_t eval(const Position& pos)
{
    int16_t score = 0;
    const int16_t color = pos.side_to_move == white ? 1 : -1;

    score = material(pos) * color;

    return score;
}