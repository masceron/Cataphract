#include <immintrin.h>

#include "accumulators.hpp"
#include "nnue.hpp"

struct Network;

unsigned char data[] = {
    #embed "../net.bin"
};

auto network = reinterpret_cast<Network*>(data);

inline void Accumulator_entry::accumulator_flip(const bool side, const Position& pos)
{
    if (side == white) is_mirrored.first = !is_mirrored.first;
    else is_mirrored.second = !is_mirrored.second;

    memcpy(&accumulators.accumulators[side], network->accumulator_biases, hl_size * sizeof(int16_t));

    for (int piece = 0; piece < 12; piece++) {
        uint64_t board = pos.boards[piece];
        while (board) {
            auto [white_add, black_add] = input_index_of(piece, least_significant_one(board), is_mirrored);
            const uint16_t add = side == white ? white_add : black_add;

            for (int iter = 0; iter < hl_size; iter++) {
                accumulators.accumulators[side][iter] += network->accumulator_weights[add][iter];
            }

            board &= board - 1;
        }
    }
}

void Accumulator_entry::mark_changes(Position& pos, const Move& move)
{
    const uint8_t from = move.src();
    const uint8_t to = move.dest();
    const uint8_t flag = move.flag();
    const uint8_t moved_piece = pos.piece_on[from];

    if ((moved_piece == K || moved_piece == k) && from % 8 / 4 != to % 8 / 4) {
        const Pieces captured = pos.piece_on[to];
        if (captured != nil) pos.remove_piece(to);
        pos.move_piece(from, to);
        if (flag == queen_castle) pos.move_piece(to - 2, to + 1);
        accumulator_flip(pos.side_to_move, pos);
        pos.move_piece(to, from);
        if (captured != nil) pos.put_piece(captured, to);
        if (flag == queen_castle) pos.move_piece(to + 1, to - 2);
    }

    if (flag < knight_promotion) {
        adds[0] = {moved_piece, to};
        subs[0] = {moved_piece, from};
        if (flag == capture) {
            subs[1] = { pos.piece_on[to], to};
        }
        else if (flag == ep_capture) {
            const uint8_t captured_sq = to - Delta<Up>(pos.side_to_move);
            subs[1] = {pos.piece_on[captured_sq], captured_sq};
        }
        else if (flag == king_castle) {
            auto rook = pos.side_to_move == white ? R : r;
            adds[1] = {rook, to - 1};
            subs[1] = {rook, to + 1};
        }
        else if (flag == queen_castle) {
            auto rook = pos.side_to_move == white ? R : r;
            adds[1] = {rook, to + 1};
            subs[1] = {rook, to - 2};
        }
    }
    else {
        subs[0] = {moved_piece, from};
        adds[0] = {move.promoted_to<true>(pos.side_to_move), to};
        if (flag >= knight_promo_capture) {
            subs[1] = {pos.piece_on[to], to};
        }
    }
}

int32_t NNUE::forward(int16_t* stm, int16_t* nstm, const uint8_t bucket)
{
    static const __m256i vec_zero = _mm256_setzero_si256();
    static const __m256i vec_QA = _mm256_set1_epi16(QA);

    auto to_move = reinterpret_cast<__m256i*>(stm);
    auto not_to_move = reinterpret_cast<__m256i*>(nstm);
    auto move_weights = reinterpret_cast<const __m256i*>(&network->output_weights[bucket]);
    auto non_move_weights = reinterpret_cast<const __m256i*>(&network->output_weights[bucket][hl_size]);

    __m256i sum = vec_zero;

    constexpr uint8_t iters = hl_size / 16;
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
    static constexpr uint8_t divisor = (32 + output_buckets - 1) / output_buckets;
    const uint8_t bucket = (std::popcount(pos.occupations[2]) - 2) / divisor;
    const int32_t eval = forward((*accumulator_pair)[pos.side_to_move], (*accumulator_pair)[!pos.side_to_move], bucket);

    return static_cast<int16_t>((eval / QA + network->output_bias[bucket]) * eval_scale / (QA * QB));
}
void NNUE::update_accumulators()
{
    accumulator_stack_update(network);
}

void NNUE::refresh_accumulators(Accumulators* accumulators, const Position& pos)
{
    accumulators_set(network, accumulators, pos);
}
