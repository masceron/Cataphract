#pragma once

#include <cstdint>

#include "../position/position.hpp"

static constexpr uint8_t QA = 255;
static constexpr uint8_t QB = 64;
static constexpr int16_t EVAL_SCALE = 400;
#define HL_SIZE 1024
#define INPUT_SIZE 768
#define OUTPUT_BUCKETS 8
#define INPUT_BUCKETS 10

struct Network
{
    int16_t accumulator_weights[INPUT_BUCKETS][INPUT_SIZE][HL_SIZE];
    int16_t accumulator_biases[HL_SIZE];
    int16_t output_weights[OUTPUT_BUCKETS][2 * HL_SIZE];
    int16_t output_bias[OUTPUT_BUCKETS];
};

struct Accumulator_entry
{
    alignas(32) int16_t accumulators[2 * HL_SIZE];
    uint64_t bitboards[12];
    bool is_dirty;
    bool require_rebuild;
    std::pair<uint8_t, int8_t> adds[2];
    std::pair<uint8_t, int8_t> subs[2];

    void mark_changes(Position& pos, Move move);
};

namespace NNUE
{
    int32_t forward(int16_t* stm, int16_t* nstm, uint8_t bucket);
    void update_accumulators();
    void refresh_accumulators(Position& pos);
    int16_t evaluate(const Position& pos, int16_t* accumulator_pair);
}