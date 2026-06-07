#pragma once

#include <cstdint>

struct Position;
struct AccumulatorStack;

void refresh_accumulators(const Position& pos, AccumulatorStack& accumulator_stack);
int16_t eval(const Position& pos, AccumulatorStack& accumulator_stack);