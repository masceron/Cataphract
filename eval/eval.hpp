#pragma once
#include <cstdint>

constexpr int white_negamax = 1, black_negamax = -1;

constexpr int16_t pawn_weight = 126;
constexpr int16_t knight_weight = 781;
constexpr int16_t bishop_weight = 825;
constexpr int16_t rook_weight = 1276;
constexpr int16_t queen_weight = 2538;

inline int16_t material()
{
    const int us = position.side_to_move;
    int mat = 0;
    mat += pawn_weight * (std::popcount(position.boards[us == white ? P : p]) - std::popcount(position.boards[us == white ? p : P]))
        + knight_weight * (std::popcount(position.boards[us == white ? N : n]) - std::popcount(position.boards[us == white ? n : N]))
        + bishop_weight * (std::popcount(position.boards[us == white ? B : b]) - std::popcount(position.boards[us == white ? b : B]))
        + rook_weight * (std::popcount(position.boards[us == white ? R : r]) - std::popcount(position.boards[us == white ? r : R]))
        + queen_weight * (std::popcount(position.boards[us == white ? Q : q]) - std::popcount(position.boards[us == white ? q : Q]));
    return mat;
}

inline int16_t eval()
{
    const bool us = position.side_to_move;
    int16_t score = 0;

    score = material() * (us == white ? white_negamax : black_negamax);

    return score;
}

