#pragma once
#include <cstdint>

#include "nnue.hpp"
#include "accumulators.hpp"

inline int16_t eval(const Position& pos)
{
    const auto back = accumulator_stack[accumulator_stack.size - 1];
    if (back->is_dirty)
    {
        NNUE::update_accumulators();
    }

    return NNUE::evaluate(pos, back->accumulators);
}
