#ifdef __AVX512F__

#include <immintrin.h>

#include "../arch.hpp"
#include "simd.hpp"

namespace NNUE
{
    int32_t forward(const Network& __restrict network, int16_t* __restrict stm, int16_t* __restrict nstm, const uint8_t bucket)
    {
        static const __m512i vec_zero = _mm512_setzero_si512();
        static const __m512i vec_QA = _mm512_set1_epi16(QA);

        auto to_move = reinterpret_cast<__m512i*>(stm);
        auto not_to_move = reinterpret_cast<__m512i*>(nstm);
        auto move_weights = reinterpret_cast<const __m512i*>(&network.output_weights[bucket]);
        auto non_move_weights = reinterpret_cast<const __m512i*>(&network.output_weights[bucket][HL_SIZE]);

        __m512i sum = vec_zero;

        static constexpr int iters = HL_SIZE / 32;

        for (int i = 0; i < iters; i++)
        {
            const __m512i us = _mm512_load_si512(to_move++);
            const __m512i them = _mm512_load_si512(not_to_move++);
            const __m512i us_weights = _mm512_load_si512(move_weights++);
            const __m512i them_weights = _mm512_load_si512(non_move_weights++);

            const __m512i us_clamped = _mm512_min_epi16(_mm512_max_epi16(us, vec_zero), vec_QA);
            const __m512i them_clamped = _mm512_min_epi16(_mm512_max_epi16(them, vec_zero), vec_QA);

            const __m512i us_results = _mm512_madd_epi16(_mm512_mullo_epi16(us_weights, us_clamped), us_clamped);
            const __m512i them_results = _mm512_madd_epi16(_mm512_mullo_epi16(them_weights, them_clamped), them_clamped);

            sum = _mm512_add_epi32(sum, us_results);
            sum = _mm512_add_epi32(sum, them_results);
        }

        return _mm512_reduce_add_epi32(sum);
    }
}

#endif