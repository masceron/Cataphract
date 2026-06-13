#include <cstring>
#include <algorithm>

#include "nnue.hpp"
#include "accumulators.hpp"
#include "simd/simd.hpp"
#include "arch.hpp"
#include "utils.hpp"
#include "../position/position.hpp"

SIMD_ALIGN inline static constexpr unsigned char data[] = {
#embed "../net.bin"
};

static const Network& network = *reinterpret_cast<const Network*>(data);

void update_accumulators(AccumulatorStack& accumulator_stack)
{
    accumulator_stack_update(network, accumulator_stack);
}

void refresh_accumulators(const Position& pos, AccumulatorStack& accumulator_stack)
{
    auto& finny_table = accumulator_stack.finny_table;
    for (auto& pair : finny_table)
    {
        for (auto& [bitboards, accumulators] : pair)
        {
            std::ranges::fill(bitboards.begin(), bitboards.end(), 0);
            std::memcpy(accumulators, network.accumulator_biases, HL_SIZE * sizeof(int16_t));
            std::memcpy(&accumulators[HL_SIZE], network.accumulator_biases, HL_SIZE * sizeof(int16_t));
        }
    }
    accumulator_stack.clear();
    accumulator_stack.push();
    const auto [w, b] = get_buckets(pos.boards);

    auto& stack_entry = accumulator_stack[0];
    auto& finny_entry = finny_table[w][b];

    std::memcpy(stack_entry.bitboards.data(), pos.boards.data(), sizeof(pos.boards));
    std::memcpy(finny_entry.bitboards.data(), pos.boards.data(), sizeof(pos.boards));

    stack_entry.is_dirty = false;
    accumulators_set(network, pos.boards, stack_entry.accumulators);
    std::memcpy(finny_entry.accumulators, stack_entry.accumulators, 2 * HL_SIZE * sizeof(int16_t));
}

int16_t evaluate(const Position& pos, int16_t* accumulator_pair)
{
    static constexpr uint8_t divisor = (32 + OUTPUT_BUCKETS - 1) / OUTPUT_BUCKETS;
    const uint8_t bucket = (std::popcount(pos.occupations[2]) - 2) / divisor;
    const int32_t evaluation = NNUE::forward(network, &accumulator_pair[pos.side_to_move * HL_SIZE],
                                       &accumulator_pair[!pos.side_to_move * HL_SIZE], bucket);

    return static_cast<int16_t>((evaluation / QA + network.output_bias[bucket]) * EVAL_SCALE / (QA * QB));
}

int16_t eval(const Position& pos, AccumulatorStack& accumulator_stack)
{
    auto& back = accumulator_stack[accumulator_stack.size - 1];
    if (back.is_dirty)
    {
        update_accumulators(accumulator_stack);
    }

    return evaluate(pos, back.accumulators);
}