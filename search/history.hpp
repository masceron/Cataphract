#pragma once

#include <cstdint>
#include <forward_list>
#include <algorithm>
#include <array>
#include <cstring>
#include <list>

#include "../eval/transposition.hpp"

namespace History
{
    inline std::array<std::array<std::array<int16_t, 64>, 64>, 2> table;

    static constexpr int16_t min_history = -8000;
    static constexpr int16_t max_history = 8000;

    inline void clear()
    {
        memset(table.data(), 0, 16384);
    }

    inline void scale_down()
    {
        for (int side = 0; side < 2; side++)
            for (int from = 0; from < 64; from++) {
                std::ranges::transform(table[side][from].begin(), table[side][from].end(), table[side][from].begin(), [](const int16_t& x) {return x >> 1;});
            }
    }

    inline void apply(const bool side, const uint8_t from, const uint8_t to, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_history, max_history);
        table[side][from][to] += clamped_bonus - table[side][from][to] * abs(clamped_bonus) / max_history;
    }

    inline void update(const std::forward_list<Move>& searched, const bool side, const uint8_t from, const uint8_t to, const uint8_t depth)
    {
        const int16_t bonus = 80 * depth - 60;
        apply(side, from, to, bonus);

        for (const Move& move: searched) {
            apply(side, move.src(), move.dest(), -bonus);
        }
        if (table[side][from][to] >= max_history) scale_down();
    }
}

namespace Killers
{
    inline std::array<std::array<Move, 2>, 127> table;
    inline void clear()
    {
        memset(table.data(), 0, table.size() * 2 * sizeof(Move));
    }
    inline void insert(const Move move, const uint8_t ply)
    {
        auto killers_of_ply = table[ply];
        if (move == killers_of_ply[0] || move == killers_of_ply[1]) return;
        killers_of_ply[1] = killers_of_ply[0];
        killers_of_ply[0] = move;
    }
    inline bool find(const Move move, const uint8_t ply)
    {
        return table[ply][0] == move || table[ply][1] == move;
    }
}

struct CaptureEntry
{
    uint8_t moved;
    uint8_t captured;
    uint8_t sq;
};

namespace Capture
{
    static constexpr int16_t min_bonus = -30000;
    static constexpr int16_t max_bonus = 30000;

    inline std::array<std::array<std::array<std::array<int16_t, 64>, 5>, 6>, 2> table;
    inline void clear()
    {
        memset(table.data(), 0, 7680);
    }
    inline void scale_down()
    {
        for (int stm = 0; stm < 2; stm++)
            for (int moved_piece = 0; moved_piece < 6; moved_piece++)
                for (int captured_piece = 0; captured_piece < 5; captured_piece++)
                    std::ranges::transform(table[stm][moved_piece][captured_piece].begin(), table[stm][moved_piece][captured_piece].end(), table[stm][moved_piece][captured_piece].begin(), [](const int16_t& x) {return x >> 1;});
    }

    inline void apply(const bool stm, const uint8_t moved_piece, const uint8_t captured_piece, const uint8_t sq, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_bonus, max_bonus);
        table[stm][moved_piece][captured_piece][sq] += clamped_bonus - table[stm][moved_piece][captured_piece][sq] * abs(clamped_bonus) / max_bonus;
    }

    inline void update(const std::forward_list<CaptureEntry>& searched, const bool stm, uint8_t moved_piece, uint8_t captured_piece, const uint8_t sq, const uint8_t depth)
    {
        if (moved_piece >= 6) moved_piece -= 6;
        if (captured_piece >= 6) captured_piece -= 6;
        const int16_t bonus = 100 * depth;
        apply(stm, moved_piece, captured_piece, sq, bonus);

        for (const auto&[_moved, _captured, _sq]: searched) {
            apply(stm, _moved, _captured, _sq, -bonus);
        }
        if (table[stm][moved_piece][captured_piece][sq] >= max_bonus) scale_down();
    }
}

namespace Corrections
{
    static constexpr int16_t correction_grain = 256;
    static constexpr int16_t correction_scale = 256;
    static constexpr int16_t correction_limit = 8192;
    static constexpr uint16_t correction_size = 16384;
    inline std::array<std::array<int16_t, correction_size>, 2> pawn_corrections;
    inline std::array<std::array<int16_t, correction_size>, 2> minor_piece_corrections;
    inline std::array<std::array<int16_t, correction_size>, 2> major_piece_corrections;

    inline uint16_t index_of(const uint64_t key)
    {
        return key & (correction_size - 1);
    }

    inline void clear()
    {
        memset(pawn_corrections.data(), 0, 2 * 16384);
        memset(minor_piece_corrections.data(), 0, 2 * correction_size);
        memset(major_piece_corrections.data(), 0, 2 * correction_size);
    }

    inline void update(const int16_t delta, const Position& pos, const uint8_t depth)
    {
        const auto minors = pos.boards[N] | pos.boards[n] | pos.boards[B] | pos.boards[b];
        const auto majors = pos.boards[Q] | pos.boards[q] | pos.boards[R] | pos.boards[r];
        const auto pawns = &pawn_corrections[pos.side_to_move][index_of(pos.boards[P] | pos.boards[p])];
        const auto minor = &minor_piece_corrections[pos.side_to_move][index_of(minors)];
        const auto major = &major_piece_corrections[pos.side_to_move][index_of(majors)];

        const int new_weight = std::min(16, depth + 1);
        const int scaled_delta = delta * correction_grain;

        const int correction_pawns = *pawns * (correction_scale - new_weight) + scaled_delta * new_weight;
        const int correction_minor = *minor * (correction_scale - new_weight) + scaled_delta * new_weight;
        const int correction_major = *major * (correction_scale - new_weight) + scaled_delta * new_weight;

        *pawns = std::clamp(static_cast<int16_t>(correction_pawns / correction_scale), static_cast<int16_t>(-correction_limit), correction_limit);
        *minor = std::clamp(static_cast<int16_t>(correction_minor / correction_scale), static_cast<int16_t>(-correction_limit), correction_limit);
        *major = std::clamp(static_cast<int16_t>(correction_major / correction_scale), static_cast<int16_t>(-correction_limit), correction_limit);
    }

    inline int16_t correct(int16_t static_eval, const Position& pos)
    {
        const auto minors = pos.boards[N] | pos.boards[n] | pos.boards[B] | pos.boards[b];
        const auto majors = pos.boards[Q] | pos.boards[q] | pos.boards[R] | pos.boards[r];

        const auto pawn_correction = pawn_corrections[pos.side_to_move][index_of(pos.boards[P] | pos.boards[p])];
        const auto minor_correction = minor_piece_corrections[pos.side_to_move][index_of(minors)];
        const auto major_correction = major_piece_corrections[pos.side_to_move][index_of(majors)];

        const int corrections = pawn_correction + minor_correction + major_correction;

        static_eval += static_cast<int16_t>(corrections / correction_grain);

        return static_eval;
    }
}

namespace Continuation
{
    inline std::vector<int16_t> search_stack(0);

    static constexpr int16_t min_continuation = -8000;
    static constexpr int16_t max_continuation = 8000;
    inline std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2> counter_moves;
    inline std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2> follow_up;

    inline void clear()
    {
        search_stack.clear();
        memset(counter_moves.data(), 0, 589824);
        memset(follow_up.data(), 0, 589824);
    }

    inline void scale_down(std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2>& table)
    {
        for (int stm = 0; stm < 2; stm++)
            for (int piece_prev = 0; piece_prev < 6; piece_prev++)
                for (int prev_to = 0; prev_to < 64; prev_to++)
                    for (int piece_now = 0; piece_now < 6; piece_now++)
                        std::ranges::transform(table[stm][piece_prev][prev_to][piece_now].begin(), table[stm][piece_prev][prev_to][piece_now].end(), table[stm][piece_prev][prev_to][piece_now].begin(), [](const int16_t& x) {return x >> 1;});
    }

    void apply(auto& table, const bool stm, const uint8_t prev_piece, const uint8_t prev_to, const uint8_t piece, const uint8_t to, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_continuation, max_continuation);
        table[stm][prev_piece][prev_to][piece][to] += clamped_bonus - table[stm][prev_piece][prev_to][piece][to] * abs(clamped_bonus) / max_continuation;
    }

    inline void update(const Position& pos, const std::forward_list<Move>& searched, const Move& move, const uint8_t depth)
    {
        const int16_t bonus = 80 * depth - 60;
        const bool stm = pos.side_to_move;
        uint8_t piece = pos.piece_on[move.src()];
        if (piece >= 6) piece -= 6;

        if (!search_stack.empty()) {
            const uint8_t prev_piece = search_stack.back() >> 6;
            const uint8_t prev_to = search_stack.back() & 0b111111;

            apply(counter_moves, stm, prev_piece, prev_to, piece, move.dest(), bonus);

            for (const auto& tmpm: searched) {
                uint8_t tmp = pos.piece_on[tmpm.src()];
                if (tmp >= 6) tmp -= 6;
                apply(counter_moves, stm, prev_piece, prev_to, tmp, tmpm.dest(), -bonus);
            }
            if (counter_moves[stm][prev_piece][prev_to][piece][move.dest()] >= max_continuation) scale_down(counter_moves);

            if (search_stack.size() >= 2) {
                const auto prev2 = search_stack[search_stack.size() - 2];
                const uint8_t prev2_piece = prev2 >> 6;
                const uint8_t prev2_to = prev2 & 0b111111;

                apply(follow_up, stm, prev2_piece, prev2_to, piece, move.dest(), bonus);

                for (const auto& tmpm: searched) {
                    uint8_t tmp = pos.piece_on[tmpm.src()];
                    if (tmp >= 6) tmp -= 6;
                    apply(follow_up, stm, prev2_piece, prev2_to, tmp, tmpm.dest(), -bonus);
                }
                if (follow_up[stm][prev2_piece][prev2_to][piece][move.dest()] >= max_continuation) scale_down(follow_up);
            }
        }
    }
}