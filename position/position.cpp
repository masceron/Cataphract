#include "position.hpp"
#include "../board/bitboard.hpp"
#include "../board/pieces/king.hpp"
#include "../board/pieces/slider.hpp"
#include "../board/pieces/knight.hpp"

bool is_square_attacked_by(const uint8_t index, const bool side)
{
    switch (side) {
        case white:
            if (knight_attack_tables[index] & position.boards[N]) return true;
            if (king_attack_tables[index] & position.boards[K]) return true;
            if (pawn_attack_tables[black][index] & position.boards[P]) return true;
            if (get_bishop_attack(index, position.occupations[2]) & (position.boards[B] | position.boards[Q])) return true;
            if (get_rook_attack(index, position.occupations[2]) & (position.boards[R] | position.boards[Q])) return true;
        break;
        case black:
            if (knight_attack_tables[index] & position.boards[n]) return true;
            if (king_attack_tables[index] & position.boards[k]) return true;
            if (pawn_attack_tables[white][index] & position.boards[p]) return true;
            if (get_bishop_attack(index, position.occupations[2]) & (position.boards[b] | position.boards[q])) return true;
            if (get_rook_attack(index, position.occupations[2]) & (position.boards[r] | position.boards[q])) return true;
        break;
    }
    return false;
}

bool is_king_in_check_by(const bool side)
{
    return is_square_attacked_by(least_significant_one(side == white ? position.boards[k] : position.boards[K]), side);
}

uint64_t get_pinned_board_of(const bool side)
{
    uint64_t attacker = 0, pinned_board = 0;
    const uint8_t king_index = least_significant_one(side == white ? position.boards[K] : position.boards[k]);

    attacker = (get_rook_attack(king_index, position.occupations[1 - side]) & (side == white ? (position.boards[r] | position.boards[q]) : (position.boards[R] | position.boards[Q])))
                 | (get_bishop_attack(king_index, position.occupations[1 - side]) & (side == white ? (position.boards[b] | position.boards[q]) : (position.boards[B] | position.boards[Q])));

    while (attacker) {
        const uint8_t sniper = least_significant_one(attacker);

        if (const uint64_t between = lines_between[sniper][king_index] & position.occupations[side]; std::has_single_bit(between))
            pinned_board |= between;

        attacker &= attacker - 1;
    }

    return pinned_board;
}

uint64_t get_checker_of(const bool side)
{
    const uint8_t king_index = least_significant_one(side == white ? position.boards[K] : position.boards[k]);
    const uint64_t checkers = (pawn_attack_tables[side][king_index] & (side == white ? position.boards[p] : position.boards[P]))
        | (knight_attack_tables[king_index] & (side == white ? position.boards[n] : position.boards[N]))
        | (get_bishop_attack(king_index, position.occupations[2]) & (side == white ? (position.boards[b] | position.boards[q]) : (position.boards[B] | position.boards[Q])))
        | (get_rook_attack(king_index, position.occupations[2]) & (side == white ? (position.boards[r] | position.boards[q]) : (position.boards[R] | position.boards[Q])));
    return checkers;
}

uint64_t get_check_blocker_of(const bool side)
{
    const uint64_t checker = get_checker_of(side);
    return checker | lines_between[least_significant_one(checker)][least_significant_one(side == white ? position.boards[K] : position.boards[k])];
}