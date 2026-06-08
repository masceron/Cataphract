#if !defined(__AVX512F__) && !defined(__AVX2__)

#include <algorithm>

#include "../arch.hpp"
#include "simd.hpp"

namespace NNUE
{
    int32_t forward(const Network& __restrict network, const int16_t* __restrict stm, const int16_t* __restrict nstm, const uint8_t bucket)
    {
        int sum = 0;
        const auto move_weights = &network.output_weights[bucket][0];
        const auto non_move_weights = &network.output_weights[bucket][HL_SIZE];

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

#endif