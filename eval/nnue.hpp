#pragma once

#include <cstdint>

#include "../position/position.hpp"

static constexpr uint8_t QA = 255;
static constexpr uint8_t QB = 64;
static constexpr int16_t eval_scale = 400;
#define hl_size 1024
#define input_size 768
#define output_buckets 8

struct Network
{
    int16_t accumulator_weights[input_size][hl_size];
    int16_t accumulator_biases[hl_size];
    int16_t output_weights[output_buckets][2 * hl_size];
    int16_t output_bias[output_buckets];
};

struct Accumulators
{
    alignas(32) int16_t accumulators[2][hl_size];

    int16_t* operator[](const uint8_t index) {return accumulators[index];}
};

struct Accumulator_entry
{
    Accumulators accumulators;
    bool is_dirty;
    std::pair<bool, bool> is_mirrored;
    std::pair<uint8_t, int16_t> adds[2] = {{0, -1}, {0, -1}};
    std::pair<uint8_t, int16_t> subs[2] = {{0, -1}, {0, -1}};

    void accumulator_flip(bool side, const Position &pos);
    void mark_changes(Position& pos, const Move move);

    Accumulator_entry() = delete;
    explicit Accumulator_entry(Position& pos, const Move move, const std::pair<bool, bool>& before)
    : is_dirty(true), is_mirrored(before)
    {
        mark_changes(pos, move);
    }

    explicit Accumulator_entry(const Accumulators& _accumulators, const Position& pos): is_dirty(false)
    {
        memcpy(&accumulators, &_accumulators, 2 * hl_size * sizeof(int16_t));
        is_mirrored = {least_significant_one(pos.boards[K]) % 8 >= 4, least_significant_one(pos.boards[k]) % 8 >= 4};
    }
};

namespace NNUE
{
    int32_t forward(int16_t* stm, int16_t* nstm, uint8_t bucket);
    void update_accumulators();
    void refresh_accumulators(Accumulators* accumulators, const Position& pos);
    int16_t evaluate(const Position& pos, Accumulators* accumulator_pair);
}