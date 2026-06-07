#pragma once

#include <cstdint>
#include <array>

#include "bitboard.hpp"

constexpr std::array<uint8_t, 64> bishop_relevancy = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

constexpr uint64_t mask_bishop_attack(const int index)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index / 8) + 1, file = (index % 8) + 1; rank <= 6 && file <= 6; rank++, file++)
    {
        attack_board |= one << (rank * 8 + file);
    }
    for (int rank = (index / 8) + 1, file = (index % 8) - 1; rank <= 6 && file >= 1; rank++, file--)
    {
        attack_board |= one << (rank * 8 + file);
    }
    for (int rank = (index / 8) - 1, file = (index % 8) + 1; rank >= 1 && file <= 6; rank--, file++)
    {
        attack_board |= one << (rank * 8 + file);
    }
    for (int rank = (index / 8) - 1, file = (index % 8) - 1; rank >= 1 && file >= 1; rank--, file--)
    {
        attack_board |= one << (rank * 8 + file);
    }
    return attack_board;
}

constexpr std::array<uint8_t, 64> rook_relevancy = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

constexpr uint64_t mask_rook_attack(const int index)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index / 8) + 1; rank <= 6; rank++)
    {
        attack_board |= one << (rank * 8 + (index % 8));
    }
    for (int rank = (index / 8) - 1; rank >= 1; rank--)
    {
        attack_board |= one << (rank * 8 + (index % 8));
    }
    for (int file = (index % 8) + 1; file <= 6; file++)
    {
        attack_board |= one << ((index / 8) * 8 + file);
    }
    for (int file = (index % 8) - 1; file >= 1; file--)
    {
        attack_board |= one << ((index / 8) * 8 + file);
    }
    return attack_board;
}

constexpr uint64_t generate_bishop_attack(const int index, const uint64_t block_table)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index / 8) + 1, file = (index % 8) + 1; rank <= 7 && file <= 7; rank++, file++)
    {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    for (int rank = (index / 8) + 1, file = (index % 8) - 1; rank <= 7 && file >= 0; rank++, file--)
    {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    for (int rank = (index / 8) - 1, file = (index % 8) + 1; rank >= 0 && file <= 7; rank--, file++)
    {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    for (int rank = (index / 8) - 1, file = (index % 8) - 1; rank >= 0 && file >= 0; rank--, file--)
    {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    return attack_board;
}

constexpr uint64_t generate_rook_attack(const int index, const uint64_t block_table)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index / 8) + 1; rank <= 7; rank++)
    {
        attack_board |= one << (rank * 8 + (index % 8));
        if ((one << (rank * 8 + (index % 8))) & block_table) break;
    }
    for (int rank = (index / 8) - 1; rank >= 0; rank--)
    {
        attack_board |= one << (rank * 8 + (index % 8));
        if ((one << (rank * 8 + (index % 8))) & block_table) break;
    }
    for (int file = (index % 8) + 1; file <= 7; file++)
    {
        attack_board |= one << ((index / 8) * 8 + file);
        if ((one << ((index / 8) * 8 + file)) & block_table) break;
    }
    for (int file = (index % 8) - 1; file >= 0; file--)
    {
        attack_board |= one << ((index / 8) * 8 + file);
        if ((one << ((index / 8) * 8 + file)) & block_table) break;
    }
    return attack_board;
}


constexpr uint64_t mask_pawn_attack(const bool side, const uint8_t index)
{
    uint64_t attack_board = 0;
    const uint64_t piece_board = (attack_board + 1) << index;
    if (side == white)
    {
        attack_board |= (piece_board >> 7) & not_a_file;
        attack_board |= (piece_board >> 9) & not_h_file;
    }
    else
    {
        attack_board |= (piece_board << 7) & not_h_file;
        attack_board |= (piece_board << 9) & not_a_file;
    }
    return attack_board;
}

constexpr std::array<std::array<uint64_t, 64>, 2> generate_pawn_attack_tables()
{
    std::array<std::array<uint64_t, 64>, 2> pawn_attacks{};
    for (int i = 0; i < 64; i++)
    {
        pawn_attacks[0][i] = mask_pawn_attack(white, i);
        pawn_attacks[1][i] = mask_pawn_attack(black, i);
    }
    return pawn_attacks;
}

constexpr std::array<std::array<uint64_t, 64>, 2> pawn_attack_tables = generate_pawn_attack_tables();

consteval uint64_t mask_knight_attack(const int index)
{
    uint64_t attack_board = 0;
    const uint64_t piece_board = (attack_board + 1) << index;
    attack_board |= (piece_board >> 17) & 0x7fffffffffff & not_h_file;
    attack_board |= (piece_board >> 15) & 0x1ffffffffffff & not_a_file;
    attack_board |= (piece_board >> 10) & 0x3fffffffffffff & not_gh_file;
    attack_board |= (piece_board >> 6) & 0x3ffffffffffffff & not_ab_file;
    attack_board |= (piece_board << 17) & not_a_file;
    attack_board |= (piece_board << 15) & not_h_file;
    attack_board |= (piece_board << 10) & not_ab_file;
    attack_board |= (piece_board << 6) & not_gh_file;
    return attack_board;
}

consteval std::array<uint64_t, 64> generate_knight_attack_tables()
{
    std::array<uint64_t, 64> knight_tables{};
    for (int i = 0; i < 64; i++)
    {
        knight_tables[i] = mask_knight_attack(i);
    }
    return knight_tables;
}

constexpr std::array<uint64_t, 64> knight_attack_tables = generate_knight_attack_tables();

consteval uint64_t mask_king_attack(const uint8_t index)
{
    uint64_t attack_board = 0;
    const uint64_t piece_board = (attack_board + 1) << index;
    attack_board |= (piece_board >> 7) & 0x1ffffffffffffff & not_a_file;
    attack_board |= (piece_board >> 8) & 0xffffffffffffff;
    attack_board |= (piece_board >> 9) & 0x7fffffffffffff & not_h_file;
    attack_board |= (piece_board >> 1) & 0x7fffffffffffffff & not_h_file;
    attack_board |= (piece_board << 7) & not_h_file;
    attack_board |= piece_board << 8;
    attack_board |= (piece_board << 9) & not_a_file;
    attack_board |= (piece_board << 1) & not_a_file;
    return attack_board;
}

consteval std::array<uint64_t, 64> generate_king_attack_tables()
{
    std::array<uint64_t, 64> king_tables{};
    for (int i = 0; i < 64; i++)
    {
        king_tables[i] = mask_king_attack(i);
    }
    return king_tables;
}

constexpr std::array<uint64_t, 64> king_attack_tables = generate_king_attack_tables();