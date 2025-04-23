#include "nnue.hpp"
#include <immintrin.h>

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "accumulators.hpp"
#include "incbin.h"

struct Network;

extern "C" {
    INCBIN(nnue, "../net.bin");
    inline auto network = reinterpret_cast<const Network*>(nnue_data);
}

int32_t NNUE::forward(int16_t* stm, int16_t* nstm)
    {
        static const __m256i vec_zero = _mm256_setzero_si256();
        static const __m256i vec_QA = _mm256_set1_epi16(QA);

        auto to_move = reinterpret_cast<__m256i*>(stm);
        auto not_to_move = reinterpret_cast<__m256i*>(nstm);
        auto move_weights = reinterpret_cast<const __m256i*>(&network->output_weights);
        auto non_move_weights = reinterpret_cast<const __m256i*>(&network->output_weights[hl_size]);

        __m256i sum = vec_zero;

        constexpr uint8_t iters = 2 * hl_size / 32;
        for (int i = 0; i < iters; i++) {
            const __m256i us = _mm256_load_si256(to_move++);
            const __m256i them = _mm256_load_si256(not_to_move++);
            const __m256i us_weights = _mm256_load_si256(move_weights++);
            const __m256i them_weights = _mm256_load_si256(non_move_weights++);

            const __m256i us_clamped = _mm256_min_epi16(_mm256_max_epi16(us, vec_zero), vec_QA);
            const __m256i them_clamped = _mm256_min_epi16(_mm256_max_epi16(them, vec_zero), vec_QA);

            const __m256i us_results   = _mm256_madd_epi16(_mm256_mullo_epi16(us_weights, us_clamped), us_clamped);
            const __m256i them_results = _mm256_madd_epi16(_mm256_mullo_epi16(them_weights, them_clamped), them_clamped);

            sum = _mm256_add_epi32(sum, us_results);
            sum = _mm256_add_epi32(sum, them_results);
        }

        const __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        const auto high64  = _mm_unpackhi_epi64(sum128, sum128);
        const auto sum64 = _mm_add_epi32(high64, sum128);
        const auto high32  = _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2));
        const auto sum32 = _mm_add_epi32(sum64, high32);
        return _mm_cvtsi128_si32(sum32);
    }

int16_t NNUE::evaluate(const Position& pos, Accumulators* accumulator_pair)
{
    const int32_t eval = forward((*accumulator_pair)[pos.side_to_move], (*accumulator_pair)[!pos.side_to_move]);

    return static_cast<int16_t>((eval / QA + network->output_bias) * eval_scale / (QA * QB));
}
void NNUE::update_accumulators()
{
    accumulator_stack_update(*network);
}

void NNUE::refresh_accumulators(Accumulators* accumulators, const Position& pos)
{
    accumulators_refresh(*network, accumulators, pos);
}
