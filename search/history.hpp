#pragma once

#include <cstdint>
#include <forward_list>
#include <array>

#include "../options.hpp"
#include "../position/move.hpp"

struct Position;

struct SearchEntry
{
    uint16_t piece_to = UINT16_MAX;
    int static_eval = score_none;
    Move excluded = null_move;
    uint8_t double_extensions = 0;
    uint8_t plies;
};

struct Killers
{
    std::array<std::array<Move, 2>, 128> table;

    void insert(Move move, int ply) const;

    [[nodiscard]] Move get(int ply, int order) const;
};

struct CaptureEntry
{
    uint8_t moved;
    uint8_t captured;
    uint8_t sq;

    CaptureEntry(uint8_t _moved, uint8_t _captured, Move move);
};

struct Capture
{
    std::array<std::array<std::array<std::array<int16_t, 64>, 5>, 6>, 2> table;

    void apply(bool stm, uint8_t moved_piece, uint8_t captured_piece, int sq,
               int16_t bonus);
    void update(const std::forward_list<CaptureEntry>& searched, bool stm, uint8_t moved_piece,
                uint8_t captured_piece, uint8_t sq, uint8_t depth);
};

struct ButterflyHistory
{
    std::array<std::array<std::array<int16_t, 64>, 64>, 2> table;

    void apply(bool side, int from, int to, int16_t bonus);
    void update(const std::forward_list<Move>& searched, bool side, int from, int to,
                uint8_t depth);
};

struct PieceToHistory
{
    std::array<std::array<std::array<int16_t, 64>, 6>, 2> table;

    void apply(bool side, Piece piece, int to, int16_t bonus);
    void update(const Position& pos, const std::forward_list<Move>& searched, bool side, Piece piece,
                int to, uint8_t depth);
};

struct Continuation
{
    std::array<std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2>, 3>
    continuation_table;

    static void apply(std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2>& table,
                      bool stm, uint8_t prev_piece, uint8_t prev_to, uint8_t piece,
                      uint8_t to, int16_t bonus);
    void update(const Position& pos, const std::forward_list<Move>& searched, Move move,
                uint8_t depth, const SearchEntry* ss);
};

struct Corrections
{
    static constexpr uint16_t correction_size = 16384;
    static constexpr uint16_t continuation_correction_size = 32768;
    static constexpr int correction_limit = 1024;

    std::array<std::array<int16_t, correction_size>, 2> pawn_corrections;
    std::array<std::array<int16_t, correction_size>, 2> white_non_pawns;
    std::array<std::array<int16_t, correction_size>, 2> black_non_pawns;
    std::array<std::array<int16_t, correction_size>, 2> major_piece_corrections;
    std::array<int16_t, continuation_correction_size> continuation_corrections;

    static uint16_t index_of(uint64_t key, uint16_t size = correction_size);
    static void apply(int16_t& entry, int bonus);
    void update(int delta, const Position& pos, uint8_t depth);
    [[nodiscard]] int correct(int static_eval, const Position& pos) const;
};

struct History
{
    Killers killers;
    ButterflyHistory butterfly_history;
    PieceToHistory piece_to_history;
    Continuation continuation;
    Capture capture;
    Corrections corrections;

    void update_quiet_histories(const Position& pos, int depth, Move picked_move, const SearchEntry* ss,
                                const std::forward_list<Move>& quiets_searched);
    void clear();
};
