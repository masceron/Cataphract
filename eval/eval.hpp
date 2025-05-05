#pragma once
#include <cstdint>

#include "nnue.hpp"
#include "accumulators.hpp"

constexpr int16_t white_negamax = 1, black_negamax = -1;

constexpr int16_t pawn_weight = 126;
constexpr int16_t knight_weight = 781;
constexpr int16_t bishop_weight = 825;
constexpr int16_t rook_weight = 1276;
constexpr int16_t queen_weight = 2538;

inline int16_t eval(const Position& pos)
{
    if (accumulator_stack.back().is_dirty) {
        NNUE::update_accumulators();
    }

    return NNUE::evaluate(pos, &accumulator_stack.back().accumulators);
}