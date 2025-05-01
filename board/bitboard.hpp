#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include "bitboard.hpp"


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

enum Direction
{
    Upright = 7,
    Up = 8,
    Upleft = 9,
};

template<const Direction direction, const bool side> uint64_t Shift(const uint64_t& board)
{
    if constexpr (direction == Up && side == white) {
        return board >> 8;
    }
    else if constexpr (direction == Up && side == black) {
        return board << 8;
    }
    else if constexpr (direction == Upleft && side == white) {
        return (board >> 9) & not_h_file;
    }
    else if constexpr (direction == Upleft && side == black) {
        return (board << 9) & not_a_file;
    }
    else if constexpr (direction == Upright && side == white) {
        return (board >> 7) & not_a_file;
    }
    else if constexpr (direction == Upright && side == black) {
        return (board << 7) & not_h_file;
    }
}

template <const Direction direction> int8_t Delta(const bool side)
{
    if constexpr (direction == Up) {
        if (side == white)
            return -8;
        return 8;
    }
    if constexpr (direction == Upleft) {
        if (side == white)
            return -9;
        return 9;
    }
    if constexpr (direction == Upright) {
        if (side == white)
            return -7;
        return 7;
    }
}

inline void set_bit(uint64_t& bitboard, const uint8_t index)
{
    bitboard = bitboard | (1ull << index);
}

inline uint8_t least_significant_one(const uint64_t bitboard)
{
    return static_cast<uint8_t>(__builtin_ctzll(bitboard));
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
    board << std::endl;
    std::cout << board.str();
}