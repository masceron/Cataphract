#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include "bitboard.hpp"
#include "lines.hpp"


constexpr uint64_t not_a_file = 0xFEFEFEFEFEFEFEFE;
constexpr uint64_t not_h_file = 0x7F7F7F7F7F7F7F7F;
constexpr uint64_t not_ab_file = 0xFCFCFCFCFCFCFCFC;
constexpr uint64_t not_gh_file = 0x3F3F3F3F3F3F3F3F;


enum Side: bool
{
    white, black
};

enum Castling_rights: uint8_t
{
    white_king = 1, white_queen = 2, black_king = 4, black_queen = 8
};

enum Pieces: uint8_t {P, N, B, R, Q, K, p, n, b, r, q, k, nil};

constexpr char piece_icons[] {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k', '.'};

inline void set_bit(uint64_t& bitboard, const uint8_t index)
{
    bitboard = bitboard | (1ull << index);
}

inline uint8_t least_significant_one(const uint64_t bitboard)
{
#ifdef __GNUC__
    return static_cast<uint8_t>(__builtin_ctzll(bitboard));
#elif _MSC_VER
    unsigned long index;
    _BitScanForward64(&index, bitboard);
    return static_cast<uint8_t>(index);
#endif
}

inline void print_bitboard(const uint64_t bitboard)
{
    std::stringstream board;
    board << "   a b c d e f g h\n";
    for (int rank = 0; rank < 8; rank++) {
        board << (8-rank) << "  ";
        for (int file = 0; file < 8; file++) {
            board << ((bitboard >> (rank * 8 + file)) & 1) << " ";
        }
        board << "\n";
    }
    board << "\n";
    std::cout << board.str();
}

inline void initialize()
{
    generate_lines();
}