#pragma once
#include <cstdint>
#include "../board/bitboard.h"
#include "../board/pieces/king.h"
#include "../board/pieces/knight.h"
#include "../board/pieces/pawn.h"
#include "../board/pieces/slider.h"

class State
{
public:
    uint8_t rule_50;
    uint8_t castling_rights;
    uint64_t key;
    int en_passant_square;
};

class Position
{
public:
    uint64_t boards[12]{};
    uint64_t occupations[3]{};
    uint64_t pinned[2]{};
    side side_to_move{};
    uint16_t half_moves{};
    uint16_t full_moves{};
    State* state{};
    Position()
    {
        new_game();
    }

    void new_game()
    {
        state = new State;
        std::fill_n(boards, 12, 0);
        std::fill_n(occupations, 3, 0);
        std::fill_n(pinned, 2, 0);
        side_to_move = white;
        state->en_passant_square = -1;
        state->castling_rights = 0;
        half_moves = 0;
        full_moves = 1;
    }
};

inline Position position;

inline bool is_square_attacked_by(const uint8_t index, const side side)
{
    switch (side) {
        case white:
            if (knight_attack_tables[index] & position.boards[N]) return true;
            if (king_attack_tables[index] & position.boards[K]) return true;
            if (pawn_attack_tables[black][index] & position.boards[P]) return true;
            if (get_bishop_attack(index, position.occupations[both]) & (position.boards[B] | position.boards[Q])) return true;
            if (get_rook_attack(index, position.occupations[both]) & (position.boards[R] | position.boards[Q])) return true;
        break;
        case black:
            if (knight_attack_tables[index] & position.boards[n]) return true;
            if (king_attack_tables[index] & position.boards[k]) return true;
            if (pawn_attack_tables[white][index] & position.boards[p]) return true;
            if (get_bishop_attack(index, position.occupations[both]) & (position.boards[b] | position.boards[q])) return true;
            if (get_rook_attack(index, position.occupations[both]) & (position.boards[r] | position.boards[q])) return true;
        break;
        default:
            return false;
    }
    return false;
}

inline bool is_king_in_check_by(const side side)
{
    return is_square_attacked_by(least_significant_one(side == white ? position.boards[k] : position.boards[K]), side);
}

inline uint64_t get_pinned_board_of(const side side)
{
    uint64_t attacker = 0, pinned_board = 0;
    const uint8_t king_index = least_significant_one(side == white ? position.boards[K] : position.boards[k]);

    attacker = (get_rook_attack(king_index, position.occupations[1 - side]) & (side == white ? (position.boards[r] | position.boards[q]) : (position.boards[R] | position.boards[Q])))
                 | (get_bishop_attack(king_index, position.occupations[1 - side]) & (side == white ? (position.boards[b] | position.boards[q]) : (position.boards[B] | position.boards[Q])));

    while (attacker) {
        const uint8_t sniper = least_significant_one(attacker);

        if (const uint64_t between = get_line_between(sniper, king_index) & position.occupations[side]; only_one_1(between))
            pinned_board |= between;

        attacker &= attacker - 1;
    }

    return pinned_board;
}