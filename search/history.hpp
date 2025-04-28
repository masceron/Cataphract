#pragma once

#include <cstdint>
#include <forward_list>
#include <algorithm>
#include <array>
#include <cstring>

#include "../eval/transposition.hpp"

namespace History
{
    inline std::array<std::array<std::array<int16_t, 64>, 64>, 2> table;

    static constexpr int16_t min_bonus = -30000;
    static constexpr int16_t max_bonus = 30000;

    inline void clear()
    {
        for (int side = 0; side < 2; side++)
            for (int from = 0; from < 64; from++)
                memset(table[side][from].data(), 0, 64 * sizeof(int16_t));
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
        const int16_t clamped_bonus = std::clamp(bonus, min_bonus, max_bonus);
        table[side][from][to] += clamped_bonus - table[side][from][to] * abs(clamped_bonus) / max_bonus;
    }

    inline void update(const std::forward_list<Move>& searched, const bool side, const uint8_t from, const uint8_t to, const uint8_t depth)
    {
        const int16_t bonus = 300 * depth - 250;
        apply(side, from, to, bonus);

        for (const Move& move: searched) {
            apply(side, move.src(), move.dest(), -bonus);
        }
        if (table[side][from][to] >= max_bonus) scale_down();
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

    inline std::array<std::array<std::array<int16_t, 64>, 12>, 12> table;
    inline void clear()
    {
        for (int moved_piece = 0; moved_piece < 12; moved_piece++)
            for (int captured_piece = 0; captured_piece < 12; captured_piece++)
                memset(table[moved_piece][captured_piece].data(), 0, 64 * sizeof(int16_t));
    }
    inline void scale_down()
    {
        for (int moved_piece = 0; moved_piece < 12; moved_piece++)
            for (int captured_piece = 0; captured_piece < 12; captured_piece++) {
                std::ranges::transform(table[moved_piece][captured_piece].begin(), table[moved_piece][captured_piece].end(), table[moved_piece][captured_piece].begin(), [](const int16_t& x) {return x >> 1;});
            }
    }

    inline void apply(const uint8_t moved_piece, const uint8_t captured_piece, const uint8_t sq, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_bonus, max_bonus);
        table[moved_piece][captured_piece][sq] += clamped_bonus - table[moved_piece][captured_piece][sq] * abs(clamped_bonus) / max_bonus;
    }

    inline void update(const std::forward_list<CaptureEntry>& searched, const uint8_t moved_piece, const uint8_t captured_piece, const uint8_t sq, const uint8_t depth)
    {
        const int16_t bonus = 100 * depth;
        apply(moved_piece, captured_piece, sq, bonus);

        for (const auto&[_moved, _captured, _sq]: searched) {
            apply(_moved, _captured, _sq, -bonus);
        }
        if (table[moved_piece][captured_piece][sq] >= max_bonus) scale_down();
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