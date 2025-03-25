#pragma once
#include <array>
#include <cstdint>

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

inline uint64_t mask_rook_attack(const int& index)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index / 8) + 1; rank <= 6; rank++) {
        attack_board |= one << (rank * 8 + (index % 8));
    }
    for (int rank = (index / 8) - 1; rank >= 1; rank--) {
        attack_board |= one << (rank * 8 + (index % 8));
    }
    for (int file = (index % 8) + 1; file <= 6; file++) {
        attack_board |= one << ((index / 8) * 8 + file);
    }
    for (int file = (index % 8) - 1; file >= 1; file--) {
        attack_board |= one << ((index / 8) * 8 + file);
    }
    return attack_board;
}

inline uint64_t generate_rook_attack(const int& index, const uint64_t& block_table)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index / 8) + 1; rank <= 7; rank++) {
        attack_board |= one << (rank * 8 + (index % 8));
        if ((one << (rank * 8 + (index % 8))) & block_table) break;
    }
    for (int rank = (index / 8) - 1; rank >= 0; rank--) {
        attack_board |= one << (rank * 8 + (index % 8));
        if ((one << (rank * 8 + (index % 8))) & block_table) break;
    }
    for (int file = (index % 8) + 1; file <= 7; file++) {
        attack_board |= one << ((index / 8) * 8 + file);
        if ((one << ((index / 8) * 8 + file)) & block_table) break;
    }
    for (int file = (index % 8) - 1; file >= 0; file--) {
        attack_board |= one << ((index / 8) * 8 + file);
        if ((one << ((index / 8) * 8 + file)) & block_table) break;
    }
    return attack_board;
}