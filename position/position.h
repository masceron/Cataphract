#pragma once
#include <cstdint>
#include <deque>

#include "../board/bitboard.h"
#include "../board/pieces/king.h"
#include "../board/pieces/knight.h"
#include "../board/pieces/pawn.h"
#include "../board/pieces/slider.h"

inline side color_of(const pieces piece)
{
    if (piece < 6) return white;
    return black;
}

struct State
{
    uint8_t rule_50;
    uint8_t castling_rights;
    int en_passant_square;
    pieces captured_piece;
    State* previous;
    uint64_t pawn_key;
    uint64_t non_pawn_key;
    uint64_t castling_key;
    uint64_t en_passant_key;
    uint64_t side_key;

    uint64_t key;

};

inline std::deque<State> states{};

class Position
{
public:
    pieces piece_on[64]{};
    uint64_t boards[12]{};
    uint64_t occupations[3]{};
    uint64_t pinned[2]{};
    uint64_t checker[2]{};
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
        constexpr State c_state{};
        states.clear();
        states.push_front(c_state);
        state = &states.at(0);
        std::fill_n(piece_on, 64, nil);
        std::fill_n(boards, 12, 0);
        std::fill_n(occupations, 3, 0);
        std::fill_n(pinned, 2, 0);
        std::fill_n(checker, 2, 0);
        side_to_move = white;
        state->en_passant_square = -1;
        state->castling_rights = 0;
        state->captured_piece = nil;
        state->previous = nullptr;
        state->non_pawn_key = 0;
        state->pawn_key = 0;
        state->castling_key = 0;
        state->en_passant_key = 0;
        state->side_key = 0;
        state->key = 0;
        half_moves = 0;
        full_moves = 1;
    }

    void move_piece(const uint8_t from, const uint8_t to)
    {
        const pieces piece = piece_on[from];
        const uint64_t ft = (1ull << from) | (1ull << to);
        boards[piece] ^= ft;
        occupations[color_of(piece)] ^= ft;
        occupations[both] ^= ft;
        piece_on[to] = piece;
        piece_on[from] = nil;
    }

    void put_piece(const pieces piece, const uint8_t sq)
    {
        piece_on[sq] = piece;
        const uint64_t board = 1ull << sq;
        occupations[color_of(piece)] |= board;
        occupations[both] |= board;
        boards[piece] |= board;
    }

    void remove_piece(const uint8_t sq)
    {
        const pieces piece = piece_on[sq];
        const uint64_t board = 1ull << sq;
        piece_on[sq] = nil;
        occupations[color_of(piece)] ^= board;
        occupations[both] ^= board;
        boards[piece] ^= board;
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

        if (const uint64_t between = lines_between[sniper][king_index] & position.occupations[side]; only_one_1(between))
            pinned_board |= between;

        attacker &= attacker - 1;
    }

    return pinned_board;
}

inline uint64_t get_checker_of(const side side)
{
    const uint8_t king_index = least_significant_one(side == white ? position.boards[K] : position.boards[k]);
    const uint64_t checkers = (pawn_attack_tables[side][king_index] & (side == white ? position.boards[p] : position.boards[P]))
        | (knight_attack_tables[king_index] & (side == white ? position.boards[n] : position.boards[N]))
        | (get_bishop_attack(king_index, position.occupations[both]) & (side == white ? (position.boards[b] | position.boards[q]) : (position.boards[B] | position.boards[Q])))
        | (get_rook_attack(king_index, position.occupations[both]) & (side == white ? (position.boards[r] | position.boards[q]) : (position.boards[R] | position.boards[Q])));
    return checkers;
}

inline uint64_t get_check_blocker_of(const side side)
{
    const uint64_t checker = get_checker_of(side);
    return checker | lines_between[least_significant_one(checker)][least_significant_one(side == white ? position.boards[K] : position.boards[k])];
}

inline void print_board()
{
    std::string lines[8];
    for (int rank = 0; rank < 8; rank++) {
        lines[rank] += std::to_string(8-rank) + "  ";
        for (int file = 0; file < 8; file++) {
            lines[rank] += ". ";
        }
        lines[rank] += "\n";
    }
    for (int i = 0; i < 12; i++) {
        uint64_t board = position.boards[i];
        while (board) {
            const auto pos = least_significant_one(board);
            lines[pos / 8][3 + (pos % 8) * 2] = piece_icons[i];

            board &= board - 1;
        }
    }

    std::cout << "   a b c d e f g h\n";
    for (const auto& i: lines) std::cout << i;
    std::cout << "\n";
}