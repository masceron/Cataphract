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
#include "../board/pieces/king.hpp"

struct Accumulator_entry;

inline bool color_of(const Pieces piece)
{
    if (piece < 6) return white;
    return black;
}

struct State
{
    uint64_t piece_key;
    uint64_t castling_key;
    uint64_t en_passant_key;
    uint64_t side_key;
    uint16_t ply;
    uint8_t castling_rights;
    uint8_t rule_50;
    int8_t en_passant_square;

    uint64_t key = 0;
    uint64_t pinned;
    uint64_t checker;
    uint64_t check_blocker;
    uint64_t attacks;
    State* previous;
    Pieces captured_piece;
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

    [[nodiscard]] bool upcoming_repetition(int ply) const;

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
            const int sniper = pop_lsb(attacker);

            if (const uint64_t between = lines_between[sniper][king_index] & occupations[side]; std::has_single_bit(between))
                pinned_board |= between;
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

    template<const bool side>
    [[nodiscard]] uint64_t get_check_blocker_of() const
    {
        return state->checker | lines_between[lsb(state->checker)][lsb(side == white ? boards[K] : boards[k])];
    }

    template<const bool side>
    [[nodiscard]] uint64_t get_attacked_map_of() const
    {
        static constexpr int enemy_rook = side == white ? r : R;
        static constexpr int enemy_bishop = side == white ? b : B;
        static constexpr int enemy_queen = side == white ? q : Q;
        static constexpr int enemy_pawn = side == white ? p : P;
        static constexpr int enemy_knight = side == white ? n : N;
        static constexpr int enemy_king = side == white ? k : K;
        static constexpr int our_king = side == white ? K : k;

        const uint64_t enemy_queen_board = boards[enemy_queen];
        const uint64_t occ = occupations[2] ^ boards[our_king];

        uint64_t attacks = 0;

        auto enemy_rook_board = enemy_queen_board | boards[enemy_rook];
        while (enemy_rook_board)
        {
            attacks |= get_rook_attack(pop_lsb(enemy_rook_board), occ);
        }

        auto enemy_bishop_board = enemy_queen_board | boards[enemy_bishop];
        while (enemy_bishop_board)
        {
            attacks |= get_bishop_attack(pop_lsb(enemy_bishop_board), occ);
        }

        auto enemy_knight_board = boards[enemy_knight];
        while (enemy_knight_board)
        {
            attacks |= knight_attack_tables[pop_lsb(enemy_knight_board)];
        }

        auto enemy_pawn_board = boards[enemy_pawn];
        while (enemy_pawn_board)
        {
            attacks |= pawn_attack_tables[!side][pop_lsb(enemy_pawn_board)];
        }

        return attacks | king_attack_tables[lsb(boards[enemy_king])];
    }

    [[nodiscard]] bool is_legal(Move move);
    [[nodiscard]] bool is_pseudo_legal(Move move) const;
    [[nodiscard]] bool is_quiet(Move move) const;

    [[nodiscard]] uint64_t construct_zobrist_key() const;

    void print_board() const;

    [[nodiscard]] std::string to_fen() const;
};

inline Position position;
