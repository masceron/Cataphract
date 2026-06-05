#pragma once

#include <immintrin.h>

#include "../arch.hpp"
#include "../utils.hpp"

#define SIMD_ALIGN alignas(64)

namespace SIMD
{
    template <const int exclude>
    void accumulators_addsub(const Network& __restrict network, const int16_t* __restrict prev, int16_t* __restrict accs,
                             const std::pair<uint8_t, int8_t>* add,
                             const std::pair<uint8_t, int8_t>* sub, const std::pair<bool, bool>& mirrors,
                             const std::pair<uint8_t, uint8_t>& buckets)
    {
        auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, mirrors);
        auto [white_sub, black_sub] = input_index_of(sub[0].first, sub[0].second, mirrors);

        constexpr int CHUNKS = HL_SIZE / 32;
        if constexpr (exclude != white)
        {
            const int16_t* w_add = network.accumulator_weights[buckets.first][white_add];
            const int16_t* w_sub = network.accumulator_weights[buckets.first][white_sub];
            for (int i = 0; i < CHUNKS; i++)
            {
                const __m512i v_prev = _mm512_load_si512(&prev[i * 32]);
                const __m512i v_add = _mm512_load_si512(&w_add[i * 32]);
                const __m512i v_sub = _mm512_load_si512(&w_sub[i * 32]);
                _mm512_store_si512(&accs[i * 32], _mm512_sub_epi16(_mm512_add_epi16(v_prev, v_add), v_sub));
            }
        }
        if constexpr (exclude != black)
        {
            const int16_t* b_add = network.accumulator_weights[buckets.second][black_add];
            const int16_t* b_sub = network.accumulator_weights[buckets.second][black_sub];
            for (int i = 0; i < CHUNKS; i++)
            {
                const __m512i v_prev = _mm512_load_si512(&prev[i * 32 + HL_SIZE]);
                const __m512i v_add = _mm512_load_si512(&b_add[i * 32]);
                const __m512i v_sub = _mm512_load_si512(&b_sub[i * 32]);
                _mm512_store_si512(&accs[i * 32 + HL_SIZE], _mm512_sub_epi16(_mm512_add_epi16(v_prev, v_add), v_sub));
            }
        }
    }

    template <const int exclude>
    void accumulators_addsub2(const Network& __restrict network, const int16_t* __restrict prev, int16_t* __restrict accs,
                              const std::pair<uint8_t, int8_t>* add,
                              const std::pair<uint8_t, int8_t>* sub, const std::pair<bool, bool>& mirrors,
                              const std::pair<uint8_t, uint8_t>& buckets)
    {
        auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, mirrors);
        auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, mirrors);
        auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, mirrors);

        constexpr int CHUNKS = HL_SIZE / 32;
        if constexpr (exclude != white)
        {
            const int16_t* w_add = network.accumulator_weights[buckets.first][white_add];
            const int16_t* w_sub1 = network.accumulator_weights[buckets.first][white_sub1];
            const int16_t* w_sub2 = network.accumulator_weights[buckets.first][white_sub2];
            for (int i = 0; i < CHUNKS; i++)
            {
                const __m512i v_prev = _mm512_load_si512(&prev[i * 32]);
                const __m512i v_add = _mm512_load_si512(&w_add[i * 32]);
                const __m512i v_sub1 = _mm512_load_si512(&w_sub1[i * 32]);
                const __m512i v_sub2 = _mm512_load_si512(&w_sub2[i * 32]);
                _mm512_store_si512(&accs[i * 32],
                                   _mm512_sub_epi16(_mm512_sub_epi16(_mm512_add_epi16(v_prev, v_add), v_sub1), v_sub2));
            }
        }
        if constexpr (exclude != black)
        {
            const int16_t* b_add = network.accumulator_weights[buckets.second][black_add];
            const int16_t* b_sub1 = network.accumulator_weights[buckets.second][black_sub1];
            const int16_t* b_sub2 = network.accumulator_weights[buckets.second][black_sub2];
            for (int i = 0; i < CHUNKS; i++)
            {
                const __m512i v_prev = _mm512_load_si512(&prev[i * 32 + HL_SIZE]);
                const __m512i v_add = _mm512_load_si512(&b_add[i * 32]);
                const __m512i v_sub1 = _mm512_load_si512(&b_sub1[i * 32]);
                const __m512i v_sub2 = _mm512_load_si512(&b_sub2[i * 32]);
                _mm512_store_si512(&accs[i * 32 + HL_SIZE],
                                   _mm512_sub_epi16(_mm512_sub_epi16(_mm512_add_epi16(v_prev, v_add), v_sub1), v_sub2));
            }
        }
    }

    template <const int exclude>
    void accumulators_add2sub2(const Network& __restrict network, const int16_t* __restrict prev, int16_t* __restrict accs,

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

        constexpr int CHUNKS = HL_SIZE / 32;
        if constexpr (exclude != white)
        {
            const int16_t* w_add1 = network.accumulator_weights[buckets.first][white_add1];
            const int16_t* w_add2 = network.accumulator_weights[buckets.first][white_add2];
            const int16_t* w_sub1 = network.accumulator_weights[buckets.first][white_sub1];
            const int16_t* w_sub2 = network.accumulator_weights[buckets.first][white_sub2];
            for (int i = 0; i < CHUNKS; i++)
            {
                const __m512i v_prev = _mm512_load_si512(&prev[i * 32]);
                const __m512i v_add1 = _mm512_load_si512(&w_add1[i * 32]);
                const __m512i v_add2 = _mm512_load_si512(&w_add2[i * 32]);
                const __m512i v_sub1 = _mm512_load_si512(&w_sub1[i * 32]);
                const __m512i v_sub2 = _mm512_load_si512(&w_sub2[i * 32]);

                const __m512i sum = _mm512_add_epi16(v_prev, _mm512_add_epi16(v_add1, v_add2));
                _mm512_store_si512(&accs[i * 32], _mm512_sub_epi16(_mm512_sub_epi16(sum, v_sub1), v_sub2));
            }
        }
        if constexpr (exclude != black)
        {
            const int16_t* b_add1 = network.accumulator_weights[buckets.second][black_add1];
            const int16_t* b_add2 = network.accumulator_weights[buckets.second][black_add2];
            const int16_t* b_sub1 = network.accumulator_weights[buckets.second][black_sub1];
            const int16_t* b_sub2 = network.accumulator_weights[buckets.second][black_sub2];
            for (int i = 0; i < CHUNKS; i++)
            {
                const __m512i v_prev = _mm512_load_si512(&prev[i * 32 + HL_SIZE]);
                const __m512i v_add1 = _mm512_load_si512(&b_add1[i * 32]);
                const __m512i v_add2 = _mm512_load_si512(&b_add2[i * 32]);
                const __m512i v_sub1 = _mm512_load_si512(&b_sub1[i * 32]);
                const __m512i v_sub2 = _mm512_load_si512(&b_sub2[i * 32]);

                const __m512i sum = _mm512_add_epi16(v_prev, _mm512_add_epi16(v_add1, v_add2));
                _mm512_store_si512(&accs[i * 32 + HL_SIZE], _mm512_sub_epi16(_mm512_sub_epi16(sum, v_sub1), v_sub2));
            }
        }
    }

    inline int32_t forward(const Network& __restrict network, int16_t* __restrict stm, int16_t* __restrict nstm, const uint8_t bucket)
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
