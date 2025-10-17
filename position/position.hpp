#pragma once

#include <cstdint>
#include <deque>
#include <bit>

#include "move.hpp"
#include "zobrist.hpp"
#include "../board/pieces/pawn.hpp"
#include "../board/pieces/slider.hpp"
#include "../board/pieces/knight.hpp"
#include "../board/lines.hpp"
#include "../board/bitboard.hpp"

struct Accumulator_entry;

inline bool color_of(const Pieces piece)
{
    if (piece < 6) return white;
    return black;
}

struct State
{
    uint16_t rule_50;
    uint8_t castling_rights;
    int8_t en_passant_square;
    uint64_t piece_key;
    uint64_t castling_key;
    uint64_t en_passant_key;
    uint64_t side_key;
    uint16_t ply;

    uint64_t key = 0;
    Pieces captured_piece;
    uint64_t pinned;
    uint64_t checker;
    uint64_t check_blocker;
    State* previous;
    int8_t repetition;
};

inline std::deque<State> states;

struct Position
{
    Pieces piece_on[64]{};
    uint64_t boards[12]{};
    uint64_t occupations[3]{};
    bool side_to_move = white;
    State* state = nullptr;
    Position()
    {
        new_game();
    }
    void new_game();

    void move_piece(int from, int to);
    void put_piece(Pieces piece, int sq);
    void remove_piece(int sq);

    void make_move(Move move, State &st);
    void unmake_move(const Move &move);
    void do_move(Move move);

    void make_null_move(State& st);
    void unmake_null_move();

    [[nodiscard]] uint64_t get_check_blocker_of(bool side) const;

    [[nodiscard]] bool upcoming_repetition(int ply) const;

    [[nodiscard]] bool is_square_attacked_by(int index, bool side) const;

    template <const bool side>
    [[nodiscard]] uint64_t get_pinned_board_of() const
    {
        uint64_t attacker = 0, pinned_board = 0;
        static constexpr uint8_t king = side == white ? K : k;
        static constexpr uint8_t rook = side == white ? r : R;
        static constexpr uint8_t bishop = side == white ? b : B;
        static constexpr uint8_t queen = side == white ? q : Q;

        const int king_index = lsb(boards[king]);

        attacker = (get_rook_attack(king_index, occupations[!side]) & (boards[rook] | boards[queen]))
                     | (get_bishop_attack(king_index, occupations[!side]) & (boards[bishop] | boards[queen]));

        while (attacker) {
            const int sniper = lsb(attacker);

            if (const uint64_t between = lines_between[sniper][king_index] & occupations[side]; std::has_single_bit(between))
                pinned_board |= between;

            attacker &= attacker - 1;
        }

        return pinned_board;
    }

    template<const bool side>
    [[nodiscard]] uint64_t get_checker_of() const
    {
        static constexpr uint8_t king = side == white ? K : k;
        static constexpr uint8_t rook = side == white ? r : R;
        static constexpr uint8_t bishop = side == white ? b : B;
        static constexpr uint8_t queen = side == white ? q : Q;
        static constexpr uint8_t pawn = side == white ? p : P;
        static constexpr uint8_t knight = side == white ? n : N;

        const uint8_t king_index = lsb(boards[king]);

        const uint64_t checkers = (pawn_attack_tables[side][king_index] & (boards[pawn]))
            | (knight_attack_tables[king_index] & boards[knight])
            | (get_bishop_attack(king_index, occupations[2]) & (boards[bishop] | boards[queen]))
            | (get_rook_attack(king_index, occupations[2]) & (boards[rook] | boards[queen]));
        return checkers;
    }

    [[nodiscard]] bool is_legal(Move move);
    [[nodiscard]] bool is_pseudo_legal(Move move);
    [[nodiscard]] bool is_quiet(Move move) const;

    [[nodiscard]] uint64_t construct_zobrist_key() const;

    void print_board() const;

    [[nodiscard]] std::string to_fen() const;
};

inline Position position;
