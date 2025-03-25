#pragma once
#include <cstdint>
#include <bit>
#include <iostream>
#include <sstream>
#include "bitboard.h"


constexpr uint64_t not_a_file = 0xFEFEFEFEFEFEFEFE;
constexpr uint64_t not_h_file = 0x7F7F7F7F7F7F7F7F;
constexpr uint64_t not_ab_file = 0xFCFCFCFCFCFCFCFC;
constexpr uint64_t not_gh_file = 0x3F3F3F3F3F3F3F3F;


enum side: uint8_t
{
    white, black, both
};

enum castling_rights: uint8_t
{
    white_king = 1,white_queen = 2, black_king = 4, black_queen = 8
  };

enum pieces: uint8_t {P, N, B, R, Q, K, p, n, b, r, q, k};

inline bool get_bit(const uint64_t& bitboard, const uint8_t& index)
{
    return (bitboard >> index) & 1ull;
}

inline void set_bit(uint64_t& bitboard, const uint8_t& index)
{
    bitboard =  bitboard | (1ull << index);
}

inline void clear_bit(uint64_t& bitboard, const uint8_t& index)
{
    bitboard = bitboard & ~(1ull << index);
}

inline uint8_t least_significant_one(const uint64_t& bitboard)
{
    return std::popcount((bitboard & -bitboard) - 1ull);
}

inline void print_board(const uint64_t& bitboard)
{
    std::stringstream board;
    board << "   a b c d e f g h\n";
    for (int rank = 0; rank < 8; rank++) {
        board << (8-rank) << "  ";
        for (int file = 0; file < 8; file++) {
            board << ((bitboard >> rank * 8 + file) & 1) << " ";
        }
        board << "\n";
    }
    board << "\n";
    std::cout << board.str();
}