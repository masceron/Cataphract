#pragma once
#include <cstdint>
#include <bit>
#include <iostream>
#include <sstream>
#include "bitboard.h"

enum side
{
    white, black, both
};

enum castling_rights
{
    white_king = 1,white_queen = 2, black_king = 4, black_queen = 8
  };

enum pieces {P, N, B, R, Q, K, p, n, b, r, q, k};

inline bool get_bit(const uint64_t& bitboard, const uint8_t& index)
{
    return (bitboard >> index) & 1;
}

inline uint64_t set_bit(const uint64_t& bitboard, const uint8_t& index)
{
    return bitboard | (1 << index);
}

inline uint64_t clear_bit(const uint64_t& bitboard, const uint8_t& index)
{
    return bitboard & ~(1 << index);
}

inline uint8_t count_set_bit(const uint64_t& bitboard)
{
    return std::popcount(bitboard);
}

inline uint64_t least_significant_one(const uint64_t& bitboard)
{
    return count_set_bit((bitboard & ~bitboard) - 1);
}

inline void print_board(const uint64_t& bitboard)
{
    std::stringstream board;
    board << "   a  b  c  d  e  f  g  h\n";
    for (int rank = 0; rank < 8; rank++) {
        board << (8-rank) << "  ";
        for (int file = 0; file < 8; file++) {
            board << ((bitboard >> rank * 8 + file) & 1) << "  ";
        }
        board << "\n";
    }
    std::cout << board.str();
}