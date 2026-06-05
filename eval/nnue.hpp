#pragma once

#include "accumulators.hpp"
#include "../position/position.hpp"

namespace NNUE
{
    void update_accumulators(AccumulatorStack& accumulator_stack);
    void refresh_accumulators(const Position& pos, AccumulatorStack& accumulator_stack);
    int16_t evaluate(const Position& pos, int16_t* accumulator_pair);
}
