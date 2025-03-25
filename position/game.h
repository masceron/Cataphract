#pragma once
#include <cstdint>

#include "../board/bitboard.h"

inline uint64_t boards[12];
inline uint64_t occupations[3];
inline uint64_t pinned;
inline side side_to_move;
inline uint8_t en_passant_square;
inline uint8_t castling_rights;
inline uint16_t half_moves;
inline uint16_t full_moves;

inline void new_game()
{
    std::fill_n(boards, 12, 0);
    std::fill_n(occupations, 3, 0);
    pinned = 0;
    side_to_move = white;
    en_passant_square = -1;
    castling_rights = 0;
    half_moves = 0;
    full_moves = 1;
}