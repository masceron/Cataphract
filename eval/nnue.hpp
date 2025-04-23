#pragma once

#include <cstdint>

#include "../board/bitboard.hpp"
#include "../position/position.hpp"

static constexpr uint8_t QA = 255;
static constexpr uint8_t QB = 64;
static constexpr int16_t eval_scale = 400;
#define hl_size 512
#define input_size 768

struct Network
{
    int16_t accumulator_weights[input_size][hl_size];
    int16_t accumulator_biases[hl_size];
    int16_t output_weights[2 * hl_size];
    int16_t output_bias;
};

struct Accumulators
{
    alignas(32) int16_t accumulators[2][hl_size];

    int16_t* operator[](const uint8_t index) {return accumulators[index];}
};

struct Accumulator_entry
{
    Accumulators accumulators;
    bool is_dirty;
    std::pair<uint8_t, int16_t> adds[2] = {{0, -1}, {0, -1}};
    std::pair<uint8_t, int16_t> subs[2] = {{0, -1}, {0, -1}};

    Accumulator_entry() = delete;

    explicit Accumulator_entry(const Position& pos, const Move& move): is_dirty(true)
    {
        mark_changes(pos, move);
    }

    explicit Accumulator_entry(const Accumulators& _accumulators): is_dirty(false)
    {
        memcpy(&accumulators, &_accumulators, 2 * hl_size * sizeof(int16_t));
    }

    void mark_changes(const Position& pos, const Move& move)
    {
        const uint8_t from = move.src();
        const uint8_t to = move.dest();
        const uint8_t flag = move.flag();
        const uint8_t moved_piece = pos.piece_on[from];

        if (flag <= knight_promotion) {
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
};

namespace NNUE
{
    int32_t forward(int16_t* stm, int16_t* nstm);
    void update_accumulators();
    void refresh_accumulators(Accumulators* accumulators, const Position& pos);
    int16_t evaluate(const Position& pos, Accumulators* accumulator_pair);
}