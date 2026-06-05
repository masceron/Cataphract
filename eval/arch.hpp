#pragma once

#include <cstdint>

#define HL_SIZE 1280
#define INPUT_SIZE 768
#define OUTPUT_BUCKETS 8
#define INPUT_BUCKETS 16

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int16_t EVAL_SCALE = 400;

struct Network
{
    int16_t accumulator_weights[INPUT_BUCKETS][INPUT_SIZE][HL_SIZE];
    int16_t accumulator_biases[HL_SIZE];
    int16_t output_weights[OUTPUT_BUCKETS][2 * HL_SIZE];
    int16_t output_bias[OUTPUT_BUCKETS];
};

inline constexpr uint8_t input_buckets_map[] = {
    0,  1,  2,  3,  3,  2,  1, 0,
    4,  5,  6,  7,  7,  6,  5, 4,
    8,  9,  10, 11, 11, 10, 9, 8,
    8,  9,  10, 11, 11, 10, 9, 8,
    12, 12, 13, 13, 13, 13, 12, 12,
    12, 12, 13, 13, 13, 13, 12, 12,
    14, 14, 15, 15, 15, 15, 14, 14,
    14, 14, 15, 15, 15, 15, 14, 14
};