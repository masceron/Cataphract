#pragma once

#include <cstdint>

enum Side: uint8_t
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
uint64_t Shift(const uint64_t& board);

template <const Direction direction> int8_t Delta(bool side);

void set_bit(uint64_t& bitboard, uint8_t index);
int lsb(uint64_t bitboard);
int pop_lsb(uint64_t& bitboard);
void print_bitboard(uint64_t bitboard);
