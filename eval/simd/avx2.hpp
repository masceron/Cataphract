#pragma once

#include <immintrin.h>

#include "../arch.hpp"
#include "../utils.hpp"

#define SIMD_ALIGN alignas(32)

namespace SIMD
{
    inline int32_t forward(const Network& __restrict network, int16_t* __restrict stm, int16_t* __restrict nstm, const uint8_t bucket)
    {
        static const __m256i vec_zero = _mm256_setzero_si256();
        static const __m256i vec_QA = _mm256_set1_epi16(QA);

        auto to_move = reinterpret_cast<__m256i*>(stm);
        auto not_to_move = reinterpret_cast<__m256i*>(nstm);
        auto move_weights = reinterpret_cast<const __m256i*>(&network.output_weights[bucket]);
        auto non_move_weights = reinterpret_cast<const __m256i*>(&network.output_weights[bucket][HL_SIZE]);

        __m256i sum = vec_zero;

        static constexpr int iters = HL_SIZE / 16;
        for (int i = 0; i < iters; i++)
        {
            const __m256i us = _mm256_load_si256(to_move++);
            const __m256i them = _mm256_load_si256(not_to_move++);
            const __m256i us_weights = _mm256_load_si256(move_weights++);
            const __m256i them_weights = _mm256_load_si256(non_move_weights++);

            const __m256i us_clamped = _mm256_min_epi16(_mm256_max_epi16(us, vec_zero), vec_QA);
            const __m256i them_clamped = _mm256_min_epi16(_mm256_max_epi16(them, vec_zero), vec_QA);

            const __m256i us_results = _mm256_madd_epi16(_mm256_mullo_epi16(us_weights, us_clamped), us_clamped);
            const __m256i them_results = _mm256_madd_epi16(_mm256_mullo_epi16(them_weights, them_clamped), them_clamped);

            sum = _mm256_add_epi32(sum, us_results);
            sum = _mm256_add_epi32(sum, them_results);
        }

        const __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        const auto high64 = _mm_unpackhi_epi64(sum128, sum128);
        const auto sum64 = _mm_add_epi32(high64, sum128);
        const auto high32 = _mm_shufflelo_epi16(sum64, 0x4e);
        const auto sum32 = _mm_add_epi32(sum64, high32);
        return _mm_cvtsi128_si32(sum32);
    }
}
