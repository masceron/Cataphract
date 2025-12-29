#include <immintrin.h>
#include <cstring>

#include "accumulators.hpp"
#include "nnue.hpp"

struct Network;

#ifdef __AVX512F__
alignas(64) unsigned char data[] = {
    #embed "../net.bin"
};
#elifdef __AVX2__
alignas(32) unsigned char data[] = {
    #embed "../net.bin"
};
#endif

inline auto network = reinterpret_cast<Network*>(data);

void Accumulator_entry::mark_changes(const Position& pos, const Move move)
{
    is_dirty = true;
    require_rebuild = false;

    adds[0] = {0, -1};
    adds[1] = {0, -1};
    subs[0] = {0, -1};
    subs[1] = {0, -1};

    const uint8_t from = move.from();
    const uint8_t to = move.to();
    const uint8_t flag = move.flag();
    const Pieces moved_piece = pos.piece_on[to];
    memcpy(bitboards, &pos.boards, 12 * sizeof(uint64_t));
    const bool just_moved = !pos.side_to_move;

    if (moved_piece == K || moved_piece == k) {
        if (const int flip = !just_moved * 56;
            input_buckets_map[from ^ flip] != input_buckets_map[to ^ flip] || ((from % 8 > 3) != (to % 8 > 3))) {
            require_rebuild = true;
        }
    }

    if (flag < knight_promotion) {
        adds[0] = {moved_piece, to};
        subs[0] = {moved_piece, from};
        if (flag == capture) {
            subs[1] = { pos.state->captured_piece, to};
        }
        else if (flag == ep_capture) {
            subs[1] = {pos.state->captured_piece, to - Delta<Up>(just_moved)};
        }
        else if (flag == king_castle) {
            auto rook = just_moved == white ? R : r;
            adds[1] = {rook, to - 1};
            subs[1] = {rook, to + 1};
        }
        else if (flag == queen_castle) {
            auto rook = just_moved == white ? R : r;
            adds[1] = {rook, to + 1};
            subs[1] = {rook, to - 2};
        }
    }
    else {
        subs[0] = {!just_moved ? P : p, from};
        adds[0] = {move.promoted_to<true>(just_moved), to};
        if (flag >= knight_promo_capture) {
            subs[1] = {pos.state->captured_piece, to};
        }
    }
}

#ifdef __AVX512F__
int32_t NNUE::forward(int16_t* stm, int16_t* nstm, const uint8_t bucket)
{
    static const __m512i vec_zero = _mm512_setzero_si512();
    static const __m512i vec_QA = _mm512_set1_epi16(QA);

    auto to_move = reinterpret_cast<__m512i*>(stm);
    auto not_to_move = reinterpret_cast<__m512i*>(nstm);
    auto move_weights = reinterpret_cast<const __m512i*>(&network->output_weights[bucket]);
    auto non_move_weights = reinterpret_cast<const __m512i*>(&network->output_weights[bucket][HL_SIZE]);

    __m512i sum = vec_zero;

    static constexpr int iters = HL_SIZE / 32;

    for (int i = 0; i < iters; i++) {
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

    // Horizontal sum of the 16 x 32-bit integers in the zmm register
    return _mm512_reduce_add_epi32(sum);
}
#elifdef __AVX2__
int32_t NNUE::forward(int16_t* stm, int16_t* nstm, const uint8_t bucket)
{
    static const __m256i vec_zero = _mm256_setzero_si256();
    static const __m256i vec_QA = _mm256_set1_epi16(QA);

    auto to_move = reinterpret_cast<__m256i*>(stm);
    auto not_to_move = reinterpret_cast<__m256i*>(nstm);
    auto move_weights = reinterpret_cast<const __m256i*>(&network->output_weights[bucket]);
    auto non_move_weights = reinterpret_cast<const __m256i*>(&network->output_weights[bucket][HL_SIZE]);

    __m256i sum = vec_zero;

    static constexpr int iters = HL_SIZE / 16;
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
    const auto high32  = _mm_shufflelo_epi16(sum64, 0x4e);
    const auto sum32 = _mm_add_epi32(sum64, high32);
    return _mm_cvtsi128_si32(sum32);
}
#endif

int16_t NNUE::evaluate(const Position& pos, int16_t* accumulator_pair)
{
    static constexpr uint8_t divisor = (32 + OUTPUT_BUCKETS - 1) / OUTPUT_BUCKETS;
    const uint8_t bucket = (std::popcount(pos.occupations[2]) - 2) / divisor;
    const int32_t eval = forward(&accumulator_pair[pos.side_to_move * HL_SIZE], &accumulator_pair[!pos.side_to_move * HL_SIZE], bucket);

    return static_cast<int16_t>((eval / QA + network->output_bias[bucket]) * EVAL_SCALE / (QA * QB));
}
void NNUE::update_accumulators()
{
    accumulator_stack_update(network);
}

void NNUE::refresh_accumulators(const Position& pos)
{
    for (auto &pair: finny_table) {
        for (auto &[bitboards, accumulators] : pair) {
            memset(bitboards, 0, 12 * sizeof(uint64_t));
            memcpy(accumulators, network->accumulator_biases, HL_SIZE * sizeof(int16_t));
            memcpy(&accumulators[HL_SIZE], network->accumulator_biases, HL_SIZE * sizeof(int16_t));
        }
    }
    accumulator_stack.clear();
    accumulator_stack.push();
    const auto [w, b] = get_buckets(pos.boards);

    const auto stack_entry = accumulator_stack[0];
    const auto finny_entry = &finny_table[w][b];

    memcpy(stack_entry->bitboards, pos.boards, 12 * sizeof(uint64_t));
    memcpy(finny_entry->bitboards, pos.boards, 12 * sizeof(uint64_t));

    stack_entry->is_dirty = false;
    accumulators_set(network, pos.boards, stack_entry->accumulators);
    memcpy(finny_entry->accumulators, stack_entry->accumulators, 2 * HL_SIZE * sizeof(int16_t));
}
