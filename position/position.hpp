#pragma once

#include <cstdint>
#include <deque>
#include <bit>

#include "move.hpp"
#include "zobrist.hpp"
#include "../board/pieces/pawn.hpp"
#include "../board/pieces/slider.hpp"
#include "../board/pieces/knight.hpp"
#include "../board/pieces/king.hpp"
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
    uint8_t ply_from_root;

    uint64_t key = 0;
    Pieces captured_piece;
    uint64_t pinned;
    uint64_t checker;
    uint64_t check_blocker;
    State* previous;
    uint8_t repetition;
};

inline std::deque<State> states(1);

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

    void new_game()
    {
        std::fill_n(piece_on, 64, nil);
        std::fill_n(boards, 12, 0);
        std::fill_n(occupations, 3, 0);
    }

    void move_piece(const uint8_t from, const uint8_t to)
    {
        const Pieces piece = piece_on[from];
        const uint64_t ft = (1ull << from) | (1ull << to);
        boards[piece] ^= ft;
        occupations[color_of(piece)] ^= ft;
        occupations[2] ^= ft;
        piece_on[to] = piece;
        piece_on[from] = nil;
    }

    void put_piece(const Pieces piece, const uint8_t sq)
    {
        piece_on[sq] = piece;
        const uint64_t board = 1ull << sq;
        occupations[color_of(piece)] |= board;
        occupations[2] |= board;
        boards[piece] |= board;
    }

    void remove_piece(const uint8_t sq)
    {
        const Pieces piece = piece_on[sq];
        const uint64_t board = 1ull << sq;
        piece_on[sq] = nil;
        occupations[color_of(piece)] ^= board;
        occupations[2] ^= board;
        boards[piece] ^= board;
    }

    void make_move(const Move &move, State &st);

    void unmake_move(const Move &move);

    void do_move(const Move& move);

    void make_null_move(State& st)
    {
        memcpy(&st, state, sizeof(State));

        st.previous = state;
        st.repetition = 1;
        st.ply_from_root++;
        st.ply++;
        st.rule_50 = 0;
        st.key ^= Zobrist::side_key;
        st.side_key ^= Zobrist::side_key;

        if (st.en_passant_square != -1) {
            st.en_passant_square = -1;
            st.key ^= st.en_passant_key;
            st.en_passant_key = 0;
        }

        side_to_move = !side_to_move;

        if (side_to_move == white) st.pinned = get_pinned_board_of<white>();
        else st.pinned = get_pinned_board_of<black>();
        state = &st;
    }

    void unmake_null_move()
    {
        state = state->previous;
        side_to_move = !side_to_move;
    }

    [[nodiscard]] bool is_square_attacked_by(const uint8_t index, const bool side) const
    {
        if (!side) {
            if (get_rook_attack(index, occupations[2]) & (boards[R] | boards[Q])) return true;
            if (get_bishop_attack(index, occupations[2]) & (boards[B] | boards[Q])) return true;
            if (knight_attack_tables[index] & boards[N]) return true;
            if (pawn_attack_tables[black][index] & boards[P]) return true;
            if (king_attack_tables[index] & boards[K]) return true;
        }
        else {
            if (get_rook_attack(index, occupations[2]) & (boards[r] | boards[q])) return true;
            if (get_bishop_attack(index, occupations[2]) & (boards[b] | boards[q])) return true;
            if (knight_attack_tables[index] & boards[n]) return true;
            if (pawn_attack_tables[white][index] & boards[p]) return true;
            if (king_attack_tables[index] & boards[k]) return true;
        }
        return false;
    }

    template <const bool side>
    [[nodiscard]] uint64_t get_pinned_board_of() const
    {
        uint64_t attacker = 0, pinned_board = 0;
        static constexpr uint8_t king = side == white ? K : k;
        static constexpr uint8_t rook = side == white ? r : R;
        static constexpr uint8_t bishop = side == white ? b : B;
        static constexpr uint8_t queen = side == white ? q : Q;

        const uint8_t king_index = least_significant_one(boards[king]);

        attacker = (get_rook_attack(king_index, occupations[!side]) & (boards[rook] | boards[queen]))
                     | (get_bishop_attack(king_index, occupations[!side]) & (boards[bishop] | boards[queen]));

        while (attacker) {
            const uint8_t sniper = least_significant_one(attacker);

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

        const uint8_t king_index = least_significant_one(boards[king]);

        const uint64_t checkers = (pawn_attack_tables[side][king_index] & (boards[pawn]))
            | (knight_attack_tables[king_index] & boards[knight])
            | (get_bishop_attack(king_index, occupations[2]) & (boards[bishop] | boards[queen]))
            | (get_rook_attack(king_index, occupations[2]) & (boards[rook] | boards[queen]));
        return checkers;
    }

    [[nodiscard]] uint64_t get_check_blocker_of(const bool side) const
    {
        return state->checker | lines_between[least_significant_one(state->checker)][least_significant_one(side == white ? boards[K] : boards[k])];
    }

    [[nodiscard]] bool is_legal(const Move& move);

    [[nodiscard]] bool is_pseudo_legal(const Move& move);

    [[nodiscard]] bool does_move_give_check(const Move& move)
    {
        const uint16_t from = move.src();
        const uint16_t to = move.dest();

        move_piece(from, to);
        if (is_square_attacked_by(least_significant_one(boards[side_to_move == white ? k : K]), side_to_move)) {
            move_piece(to, from);
            return true;
        }
        move_piece(to, from);
        return false;
    }

    [[nodiscard]] uint64_t construct_zobrist_key() const;

    void print_board() const;
    std::string to_fen() const;
};

inline Position position;
