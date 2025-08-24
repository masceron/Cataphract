#pragma once

#include <cstdint>

#include "../position/position.hpp"

static constexpr uint8_t QA = 255;
static constexpr uint8_t QB = 64;
static constexpr int16_t eval_scale = 400;
#define hl_size 1024
#define input_size 768
#define output_buckets 8
#define input_buckets 10

struct Network
{
    int16_t accumulator_weights[input_buckets][input_size][hl_size];
    int16_t accumulator_biases[hl_size];
    int16_t output_weights[output_buckets][2 * hl_size];
    int16_t output_bias[output_buckets];
};

Network* get_net();

struct Accumulator_entry
{
    bool is_dirty = false;
    bool require_rebuild = false;
    uint64_t bitboards[12];
    std::pair<uint8_t, int8_t> adds[2] = {{0, -1}, {0, -1}};
    std::pair<uint8_t, int8_t> subs[2] = {{0, -1}, {0, -1}};
    alignas(32) int16_t accumulators[2 * hl_size];

    void mark_changes(Position& pos, Move move);

    Accumulator_entry(Position& pos, const Move move)
    : is_dirty(true)
    {
        mark_changes(pos, move);
    }

    Accumulator_entry() {}
};

namespace NNUE
{
    int32_t forward(int16_t* stm, int16_t* nstm, uint8_t bucket);
    void update_accumulators();
    void refresh_accumulators(Position& pos);
    int16_t evaluate(const Position& pos, int16_t* accumulator_pair);
}