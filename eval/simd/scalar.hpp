#pragma once

#include "../arch.hpp"
#include "../utils.hpp"

#define SIMD_ALIGN alignas(16)

namespace SIMD
{
    template <const int exclude>
    void accumulators_addsub(Network* __restrict network, const int16_t* __restrict prev, int16_t* __restrict accs,
                             const std::pair<uint8_t, int8_t>* add,
                             const std::pair<uint8_t, int8_t>* sub, const std::pair<bool, bool>& mirrors,
                             const std::pair<uint8_t, uint8_t>& buckets)
    {
        auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, mirrors);
        auto [white_sub, black_sub] = input_index_of(sub[0].first, sub[0].second, mirrors);

        for (int iter = 0; iter < HL_SIZE; iter++)
        {
            if constexpr (exclude != white)
            {
                accs[iter] = prev[iter]
                    + network->accumulator_weights[buckets.first][white_add][iter]
                    - network->accumulator_weights[buckets.first][white_sub][iter];
            }

            if constexpr (exclude != black)
            {
                accs[iter + HL_SIZE] = prev[iter + HL_SIZE]
                    + network->accumulator_weights[buckets.second][black_add][iter]
                    - network->accumulator_weights[buckets.second][black_sub][iter];
            }
        }
    }

    template <const int exclude>
    void accumulators_addsub2(Network* __restrict network, const int16_t* __restrict prev, int16_t* __restrict accs,
                              const std::pair<uint8_t, int8_t>* add,
                              const std::pair<uint8_t, int8_t>* sub, const std::pair<bool, bool>& mirrors,
                              const std::pair<uint8_t, uint8_t>& buckets)
    {
        auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, mirrors);
        auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, mirrors);
        auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, mirrors);

        for (int iter = 0; iter < HL_SIZE; iter++)
        {
            if constexpr (exclude != white)
            {
                accs[iter] = prev[iter]
                    + network->accumulator_weights[buckets.first][white_add][iter]
                    - network->accumulator_weights[buckets.first][white_sub1][iter]
                    - network->accumulator_weights[buckets.first][white_sub2][iter];
            }

            if constexpr (exclude != black)
            {
                accs[iter + HL_SIZE] = prev[iter + HL_SIZE]
                    + network->accumulator_weights[buckets.second][black_add][iter]
                    - network->accumulator_weights[buckets.second][black_sub1][iter]
                    - network->accumulator_weights[buckets.second][black_sub2][iter];
            }
        }
    }

    template <const int exclude>
    void accumulators_add2sub2(Network* __restrict network, const int16_t* __restrict prev, int16_t* __restrict accs,

                               const std::pair<uint8_t, int8_t>* add,

                               const std::pair<uint8_t, int8_t>* sub,
                               const std::pair<bool, bool>& mirrors,

                               const std::pair<uint8_t, uint8_t>& buckets
    )
    {
        auto [white_add1, black_add1] = input_index_of(add[0].first, add[0].second, mirrors);
        auto [white_add2, black_add2] = input_index_of(add[1].first, add[1].second, mirrors);
        auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, mirrors);
        auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, mirrors);

        for (int iter = 0; iter < HL_SIZE; iter++)
        {
            if constexpr (exclude != white)
            {
                accs[iter] = prev[iter]
                    + network->accumulator_weights[buckets.first][white_add1][iter]
                    + network->accumulator_weights[buckets.first][white_add2][iter]
                    - network->accumulator_weights[buckets.first][white_sub1][iter]
                    - network->accumulator_weights[buckets.first][white_sub2][iter];
            }

            if constexpr (exclude != black)
            {
                accs[iter + HL_SIZE] = prev[iter + HL_SIZE]
                    + network->accumulator_weights[buckets.second][black_add1][iter]
                    + network->accumulator_weights[buckets.second][black_add2][iter]
                    - network->accumulator_weights[buckets.second][black_sub1][iter]
                    - network->accumulator_weights[buckets.second][black_sub2][iter];
            }
        }
    }

    inline int32_t forward(const Network* __restrict network, const int16_t* __restrict stm, const int16_t* __restrict nstm, const uint8_t bucket)
    {
        int sum = 0;
        const auto move_weights = &network->output_weights[bucket][0];
        const auto non_move_weights = &network->output_weights[bucket][HL_SIZE];

        auto screlu = [](const int value)
        {
            const auto clamped = std::clamp(value, 0, QA);
            return clamped * clamped;
        };

        for (int i = 0; i < HL_SIZE; i++)
        {
            sum += screlu(stm[i]) * static_cast<int>(move_weights[i]);
            sum += screlu(nstm[i]) * static_cast<int>(non_move_weights[i]);
        }

        return sum;
    }
}
