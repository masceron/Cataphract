#pragma once
#include <cstdint>

constexpr int white_negamax = 1, black_negamax = -1;

constexpr int16_t pawn_weight = 126;
constexpr int16_t knight_weight = 781;
constexpr int16_t bishop_weight = 825;
constexpr int16_t rook_weight = 1276;
constexpr int16_t queen_weight = 2538;

inline int32_t material(const Position& pos)
{
    const int us = pos.side_to_move;
    return pawn_weight * (std::popcount(pos.boards[us == white ? P : p]) - std::popcount(pos.boards[us == white ? p : P]))
        + knight_weight * (std::popcount(pos.boards[us == white ? N : n]) - std::popcount(pos.boards[us == white ? n : N]))
        + bishop_weight * (std::popcount(pos.boards[us == white ? B : b]) - std::popcount(pos.boards[us == white ? b : B]))
        + rook_weight * (std::popcount(pos.boards[us == white ? R : r]) - std::popcount(pos.boards[us == white ? r : R]))
        + queen_weight * (std::popcount(pos.boards[us == white ? Q : q]) - std::popcount(pos.boards[us == white ? q : Q]));
}

inline int32_t eval(const Position& pos)
{
    const bool us = pos.side_to_move;
    int32_t score = 0;

    score = material(pos) * (us == white ? white_negamax : black_negamax);

    return score;
}

