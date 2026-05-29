#pragma once

#include <cstdint>
#include <cstring>

#include "accumulators.hpp"
#include "../position/position.hpp"

SIMD_ALIGN inline static unsigned char net_data[] = {
#embed "../net.bin"
};

namespace NNUE
{
    inline auto network = reinterpret_cast<Network*>(net_data);

    inline void update_accumulators(AccumulatorStack& accumulator_stack)
    {
        accumulator_stack_update(network, accumulator_stack);
    }

    inline void refresh_accumulators(const Position& pos, AccumulatorStack& accumulator_stack)
    {
        auto& finny_table = accumulator_stack.finny_table;
        for (auto& pair : finny_table)
        {
            for (auto& [bitboards, accumulators] : pair)
            {
                std::memset(bitboards, 0, 12 * sizeof(uint64_t));
                std::memcpy(accumulators, network->accumulator_biases, HL_SIZE * sizeof(int16_t));
                std::memcpy(&accumulators[HL_SIZE], network->accumulator_biases, HL_SIZE * sizeof(int16_t));
            }
        }
        accumulator_stack.clear();
        accumulator_stack.push();
        const auto [w, b] = get_buckets(pos.boards);

        const auto stack_entry = accumulator_stack[0];
        const auto finny_entry = &finny_table[w][b];

        std::memcpy(stack_entry->bitboards, pos.boards, 12 * sizeof(uint64_t));
        std::memcpy(finny_entry->bitboards, pos.boards, 12 * sizeof(uint64_t));

        stack_entry->is_dirty = false;
        accumulators_set(network, pos.boards, stack_entry->accumulators);
        std::memcpy(finny_entry->accumulators, stack_entry->accumulators, 2 * HL_SIZE * sizeof(int16_t));
    }

    inline int16_t evaluate(const Position& pos, int16_t* accumulator_pair)
    {
        static constexpr uint8_t divisor = (32 + OUTPUT_BUCKETS - 1) / OUTPUT_BUCKETS;
        const uint8_t bucket = (std::popcount(pos.occupations[2]) - 2) / divisor;
        const int32_t eval = SIMD::forward(network, &accumulator_pair[pos.side_to_move * HL_SIZE],
                                           &accumulator_pair[!pos.side_to_move * HL_SIZE], bucket);

        return static_cast<int16_t>((eval / QA + network->output_bias[bucket]) * EVAL_SCALE / (QA * QB));
    }
}
