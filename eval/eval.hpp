#pragma once
#include <cstdint>

#include "accumulators.hpp"

constexpr int16_t white_negamax = 1, black_negamax = -1;

constexpr int16_t pawn_weight = 100;
constexpr int16_t knight_weight = 325;
constexpr int16_t bishop_weight = 325;
constexpr int16_t rook_weight = 500;
constexpr int16_t queen_weight = 1050;

inline int16_t eval(const Position& pos)
{
    if (accumulator_stack.back().is_dirty) {
        NNUE::update_accumulators();
    }

    return NNUE::evaluate(pos, &accumulator_stack.back().accumulators);
}