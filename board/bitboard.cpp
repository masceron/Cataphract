#include <bit>
#include <print>

#include "bitboard.hpp"

void set_bit(uint64_t& bitboard, const uint8_t index)
{
    bitboard = bitboard | (1ull << index);
}

int lsb(const uint64_t bitboard)
{
    [[assume(bitboard != 0)]];
    return std::countr_zero(bitboard);
}

int pop_lsb(uint64_t& bitboard)
{
    const int sq = lsb(bitboard);
    bitboard &= bitboard - 1;
    return sq;
}

void print_bitboard(const uint64_t bitboard)
{
    std::print("   a b c d e f g h\n");
    for (int rank = 0; rank < 8; rank++)
    {
        std::print("{}  ", 8 - rank);
        for (int file = 0; file < 8; file++)
        {
            std::print("{} ", bitboard >> (rank * 8 + file) & 1);
        }
        std::println();
    }
}
