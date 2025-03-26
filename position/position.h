#pragma once
#include <cstdint>
#include "game.h"
#include "../board/bitboard.h"
#include "../board/pieces/king.h"
#include "../board/pieces/knight.h"
#include "../board/pieces/pawn.h"
#include "../board/pieces/slider.h"

inline bool is_square_attacked_by(const uint8_t index, const side side)
{
    switch (side) {
        case white:
            if (knight_attack_tables[index] & boards[N]) return true;
            if (king_attack_tables[index] & boards[K]) return true;
            if (pawn_attack_tables[black][index] & boards[P]) return true;
            if (get_bishop_attack(index, occupations[both]) & (boards[B] | boards[Q])) return true;
            if (get_rook_attack(index, occupations[both]) & (boards[R] | boards[Q])) return true;
        break;
        case black:
            if (knight_attack_tables[index] & boards[n]) return true;
            if (king_attack_tables[index] & boards[k]) return true;
            if (pawn_attack_tables[white][index] & boards[p]) return true;
            if (get_bishop_attack(index, occupations[both]) & (boards[b] | boards[q])) return true;
            if (get_rook_attack(index, occupations[both]) & (boards[r] | boards[q])) return true;
        break;
    }
    return false;
}

inline bool is_king_in_check_by(const side side)
{
    return is_square_attacked_by(least_significant_one(side == white ? boards[k] : boards[K]), side);
}

inline uint64_t get_pinned_board_of(const side side)
{
    uint64_t attacker = 0, pinned_board = 0;
    const uint8_t king_index = least_significant_one(side == white ? boards[K] : boards[k]);

    attacker = (get_rook_attack(king_index, occupations[1 - side]) & (side == white ? (boards[r] | boards[q]) : (boards[R] | boards[Q])))
                 | (get_bishop_attack(king_index, occupations[1 - side]) & (side == white ? (boards[b] | boards[q]) : (boards[B] | boards[Q])));

    while (attacker) {
        const uint8_t sniper = least_significant_one(attacker);

        if (const uint64_t between = get_line_between(sniper, king_index) & occupations[side]; only_one_1(between))
            pinned_board |= between;

        attacker &= attacker - 1;
    }

    return pinned_board;
}