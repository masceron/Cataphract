#pragma once

#include <array>

#include "simd/simd.hpp"

struct Move;
struct Position;

struct AccumulatorEntry
{
    SIMD_ALIGN int16_t accumulators[2 * HL_SIZE];
    std::array<uint64_t, 14> bitboards;
    std::pair<uint8_t, int8_t> adds[2];
    std::pair<uint8_t, int8_t> subs[2];
    bool is_dirty;
    bool require_rebuild;

    void mark_changes(const Position& pos, Move move);
};

struct FinnyEntry
{
    std::array<uint64_t, 14> bitboards;
    SIMD_ALIGN int16_t accumulators[2 * HL_SIZE];
};

struct AccumulatorStack
{
    AccumulatorEntry stack[130];
    int size;
    FinnyEntry finny_table[INPUT_BUCKETS][INPUT_BUCKETS];

    AccumulatorStack();
    void push(const Position& pos, Move move);
    void push();
    void clear();
    void pop();
    AccumulatorEntry& operator[](int idx);
};

void accumulators_set(const Network& __restrict network, const std::array<uint64_t, 14>& boards,
                             int16_t* __restrict accumulators);

void accumulator_stack_update(const Network& __restrict network, AccumulatorStack& accumulator_stack);