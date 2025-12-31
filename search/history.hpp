#pragma once

#include <cstdint>
#include <forward_list>
#include <algorithm>
#include <array>
#include <cstring>

#include "../eval/transposition.hpp"

struct SearchEntry
{
    uint16_t piece_to = UINT16_MAX;
    int16_t static_eval = score_none;
    uint8_t plies;
};

inline void search_stack_init(std::vector<SearchEntry>& stack)
{
    for (unsigned long long i = 0; i < stack.size(); i++) {
        stack[i].plies = i - 4;
    }
}

namespace Killers
{
    inline std::array<std::array<Move, 2>, 128> table;
    inline void clear()
    {
        memset(table.data(), 0, table.size() * 2 * sizeof(Move));
    }
    inline void insert(const Move move, const int ply)
    {
        auto killers_of_ply = table[ply];
        if (move == killers_of_ply[0] || move == killers_of_ply[1]) return;
        killers_of_ply[1] = killers_of_ply[0];
        killers_of_ply[0] = move;
    }
    inline bool find(const Move move, const int ply)
    {
        return table[ply][0] == move || table[ply][1] == move;
    }
}

struct CaptureEntry
{
    uint8_t moved;
    uint8_t captured;
    uint8_t sq;

    CaptureEntry(const uint8_t _moved, const uint8_t _captured, const uint8_t _sq): moved(_moved), captured(_captured), sq(_sq)
    {
        if (captured >= 6) captured -= 6;
    }
};

namespace Capture
{
    static constexpr int16_t min_capture = -32000;
    static constexpr int16_t max_capture = 32000;

    inline std::array<std::array<std::array<std::array<int16_t, 64>, 5>, 6>, 2> table;
    inline void clear()
    {
        memset(table.data(), 0, 7680);
    }
    inline void scale_down()
    {
        for (auto& sides: table) {
            for (auto& capturers: sides) {
                for (auto& captured: capturers) {
                    for (auto& sq: captured) {
                        sq >>= 1;
                    }
                }
            }
        }
    }

    inline void apply(const bool stm, const uint8_t moved_piece, const uint8_t captured_piece, const int sq, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_capture, max_capture);
        table[stm][moved_piece][captured_piece][sq] += clamped_bonus - table[stm][moved_piece][captured_piece][sq] * abs(clamped_bonus) / max_capture;
    }

    inline void update(const std::forward_list<CaptureEntry>& searched, const bool stm, const uint8_t moved_piece, uint8_t captured_piece, const uint8_t sq, const uint8_t depth)
    {
        if (captured_piece >= 6) captured_piece -= 6;
        const int16_t bonus = 100 * depth;
        apply(stm, moved_piece, captured_piece, sq, bonus);

        for (const auto&[_moved, _captured, _sq]: searched) {
            apply(stm, _moved, _captured, _sq, -bonus);
        }
        if (table[stm][moved_piece][captured_piece][sq] >= max_capture) scale_down();
    }
}

namespace ButterflyHistory
{
    inline std::array<std::array<std::array<int16_t, 64>, 64>, 2> table;

    static constexpr int16_t min_butterfly = -8000;
    static constexpr int16_t max_butterfly = 8000;

    inline void clear()
    {
        memset(table.data(), 0, 16384);
    }

    inline void scale_down()
    {
        for (auto& sides: table) {
            for (auto& sqf: sides) {
                for (auto& sqt: sqf) {
                    sqt >>= 1;
                }
            }
        }
    }

    inline void apply(const bool side, const int from, const int to, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_butterfly, max_butterfly);
        table[side][from][to] += clamped_bonus - table[side][from][to] * abs(clamped_bonus) / max_butterfly;
    }

    inline void update(const std::forward_list<Move>& searched, const bool side, const int from, const int to, const uint8_t depth)
    {
        const int16_t bonus = 25 * depth - 12;
        apply(side, from, to, bonus);

        for (const Move move: searched) {
            apply(side, move.from(), move.to(), -bonus);
        }
        if (table[side][from][to] >= max_butterfly) scale_down();
    }
}

namespace PieceToHistory
{
    inline std::array<std::array<std::array<int16_t, 64>, 6>, 2> table;

    static constexpr int16_t min_piece_to = -8000;
    static constexpr int16_t max_piece_to = 8000;

    inline void clear()
    {
        memset(table.data(), 0, 1536);
    }

    inline void scale_down()
    {
        for (auto& sides: table) {
            for (auto& pieces: sides) {
                for (auto& sqt: pieces) {
                    sqt >>= 1;
                }
            }
        }
    }

    inline void apply(const bool side, const Pieces piece, const int to, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_piece_to, max_piece_to);
        table[side][piece][to] += clamped_bonus - table[side][piece][to] * abs(clamped_bonus) / max_piece_to;
    }

    inline void update(const Position& pos, const std::forward_list<Move>& searched, const bool side, Pieces piece, const int to, const uint8_t depth)
    {
        if (piece >= 6) piece = static_cast<Pieces>(static_cast<uint8_t>(piece) - 6);
        const int16_t bonus = 25 * depth - 12;
        apply(side, piece, to, bonus);

        for (const Move move: searched) {
            uint8_t p = pos.piece_on[move.from()];
            if (p >= 6) p -= 6;

            apply(side, static_cast<Pieces>(p), move.to(), -bonus);
        }
        if (table[side][piece][to] >= max_piece_to) scale_down();
    }
}

namespace Continuation
{
    static constexpr int16_t min_continuation = -12000;
    static constexpr int16_t max_continuation = 12000;
    inline std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2> counter_moves;
    inline std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2> follow_up;

    inline void clear()
    {
        memset(counter_moves.data(), 0, 589824);
        memset(follow_up.data(), 0, 589824);
    }

    inline void scale_down(std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2>& table)
    {
        for (auto& sides: table) {
            for (auto& piece_prev: sides) {
                for (auto& prev_to: piece_prev) {
                    for (auto& pto: prev_to) {
                        for (auto& to: pto) {
                            to >>= 1;
                        }
                    }
                }
            }
        }
    }

    void apply(auto& table, const bool stm, const uint8_t prev_piece, const uint8_t prev_to, const uint8_t piece, const uint8_t to, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_continuation, max_continuation);
        table[stm][prev_piece][prev_to][piece][to] += clamped_bonus - table[stm][prev_piece][prev_to][piece][to] * abs(clamped_bonus) / max_continuation;
    }

    inline void update(const Position& pos, const std::forward_list<Move>& searched, const Move move, const uint8_t depth, const SearchEntry* ss)
    {
        const int16_t bonus = 25 * depth - 12;
        const bool stm = pos.side_to_move;
        uint8_t piece = pos.piece_on[move.from()];
        if (piece >= 6) piece -= 6;

        if (const auto prev = ss - 1; (ss - 1)->piece_to != UINT16_MAX) {
            const uint8_t prev_piece = prev->piece_to >> 6;
            const uint8_t prev_to = prev->piece_to & 0b111111;

            apply(counter_moves, stm, prev_piece, prev_to, piece, move.to(), bonus);

            for (const auto& tmpm: searched) {
                uint8_t tmp = pos.piece_on[tmpm.from()];
                if (tmp >= 6) tmp -= 6;
                apply(counter_moves, stm, prev_piece, prev_to, tmp, tmpm.to(), -bonus);
            }
            if (counter_moves[stm][prev_piece][prev_to][piece][move.to()] >= max_continuation) scale_down(counter_moves);
        }

        if (const auto prev2 = ss - 2; (ss - 2)->piece_to != UINT16_MAX) {
            const uint8_t prev2_piece = prev2->piece_to >> 6;
            const uint8_t prev2_to = prev2->piece_to & 0b111111;

            apply(follow_up, stm, prev2_piece, prev2_to, piece, move.to(), bonus);

            for (const auto& tmpm: searched) {
                uint8_t tmp = pos.piece_on[tmpm.from()];
                if (tmp >= 6) tmp -= 6;
                apply(follow_up, stm, prev2_piece, prev2_to, tmp, tmpm.to(), -bonus);
            }
            if (follow_up[stm][prev2_piece][prev2_to][piece][move.to()] >= max_continuation) scale_down(follow_up);
        }
    }
}

inline void update_quiet_histories(const Position& pos, const int depth, const Move picked_move, const SearchEntry* ss, const std::forward_list<Move>& quiets_searched)
{
    Killers::insert(picked_move, ss->plies);
    ButterflyHistory::update(quiets_searched, pos.side_to_move, picked_move.from(), picked_move.to(),
                    depth);
    PieceToHistory::update(pos, quiets_searched, pos.side_to_move, pos.piece_on[picked_move.from()], picked_move.to(), depth);
    Continuation::update(pos, quiets_searched, picked_move, depth, ss);
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