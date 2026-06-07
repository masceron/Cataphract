#include <bit>
#include <print>

#include "bitboard.hpp"

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
        return (board >> 9) & 0x7F7F7F7F7F7F7F7F;
    }
    else if constexpr (direction == Direction::Upleft && side == black)
    {
        return (board << 9) & 0xFEFEFEFEFEFEFEFE;
    }
    else if constexpr (direction == Direction::Upright && side == white)
    {
        return (board >> 7) & 0xFEFEFEFEFEFEFEFE;
    }
    else if constexpr (direction == Direction::Upright && side == black)
    {
        return (board << 7) & 0x7F7F7F7F7F7F7F7F;
    }
}

template uint64_t Shift<Direction::Up, false>(const uint64_t&);
template uint64_t Shift<Direction::Upleft, false>(const uint64_t&);
template uint64_t Shift<Direction::Upright, false>(const uint64_t&);
template uint64_t Shift<Direction::Up, true>(const uint64_t&);
template uint64_t Shift<Direction::Upleft, true>(const uint64_t&);
template uint64_t Shift<Direction::Upright, true>(const uint64_t&);

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

template int8_t Delta<Direction::Upleft>(bool);
template int8_t Delta<Direction::Upright>(bool);
template int8_t Delta<Direction::Up>(bool);

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
