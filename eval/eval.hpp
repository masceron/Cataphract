#pragma once
#include <cstdint>

#include "nnue.hpp"
#include "accumulators.hpp"

inline constexpr int16_t pawn_weight = 126;
inline constexpr int16_t knight_weight = 781;
inline constexpr int16_t bishop_weight = 825;
inline constexpr int16_t rook_weight = 1276;
inline constexpr int16_t queen_weight = 2538;

inline int16_t eval(const Position& pos)
{
    const auto back = accumulator_stack[accumulator_stack.size - 1];
    if (back->is_dirty) {
        NNUE::update_accumulators();
    }

    return NNUE::evaluate(pos, back->accumulators);
}