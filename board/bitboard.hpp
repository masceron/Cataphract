#pragma once

#include <cstdint>

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

enum PieceType: uint8_t {Pawn, Knight, Bishop, Rook, Queen, King, None};

enum Piece: uint8_t { P, N, B, R, Q, K, p = 8, n, b, r, q, k, nil };

inline PieceType type_of(const Piece piece)
{
    return static_cast<PieceType>(piece & 7);
}

constexpr char piece_icons[]{'P', 'N', 'B', 'R', 'Q', 'K', '.', '.', 'p', 'n', 'b', 'r', 'q', 'k', '.'};

enum class Direction
{
    Upright = 7,
    Up = 8,
    Upleft = 9,
};

template <const Direction direction, const bool side>
uint64_t Shift(const uint64_t& board)
{
    if constexpr (direction == Direction::Up && side == white)
    {
        return board >> 8;
    }
    else if constexpr (direction == Direction::Up && side == black)
    {
        return board << 8;
    }
    else if constexpr (direction == Direction::Upleft && side == white)
    {
        return (board >> 9) & not_h_file;
    }
    else if constexpr (direction == Direction::Upleft && side == black)
    {
        return (board << 9) & not_a_file;
    }
    else if constexpr (direction == Direction::Upright && side == white)
    {
        return (board >> 7) & not_a_file;
    }
    else if constexpr (direction == Direction::Upright && side == black)
    {
        return (board << 7) & not_h_file;
    }
}

template <const Direction direction>
int8_t Delta(const bool side)
{
    if constexpr (direction == Direction::Up)
    {
        if (side == white)
            return -8;
        return 8;
    }
    if constexpr (direction == Direction::Upleft)
    {
        if (side == white)
            return -9;
        return 9;
    }
    if constexpr (direction == Direction::Upright)
    {
        if (side == white)
            return -7;
        return 7;
    }
}

void set_bit(uint64_t& bitboard, uint8_t index);
int lsb(uint64_t bitboard);
int pop_lsb(uint64_t& bitboard);
void print_bitboard(uint64_t bitboard);
