#pragma once
#include <array>
#include <cstdint>

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

inline uint64_t mask_bishop_attack(const int index)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index/8) + 1, file = (index % 8) + 1; rank <= 6 && file <= 6; rank++, file++) {
        attack_board |= one << (rank * 8 + file);
    }
    for (int rank = (index/8) + 1, file = (index % 8) - 1; rank <= 6 && file >= 1; rank++, file--) {
        attack_board |= one << (rank * 8 + file);
    }
    for (int rank = (index/8) - 1, file = (index % 8) + 1; rank >= 1 && file <= 6; rank--, file++) {
        attack_board |= one << (rank * 8 + file);
    }
    for (int rank = (index/8) - 1, file = (index % 8) - 1; rank >= 1 && file >= 1 ; rank--, file--) {
        attack_board |= one << (rank * 8 + file);
    }
    return attack_board;
}

inline uint64_t generate_bishop_attack(const int index, const uint64_t block_table)
{
    uint64_t attack_board = 0;
    constexpr uint64_t one = 1;
    for (int rank = (index/8) + 1, file = (index % 8) + 1; rank <= 7 && file <= 7; rank++, file++) {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    for (int rank = (index/8) + 1, file = (index % 8) - 1; rank <= 7 && file >= 0; rank++, file--) {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    for (int rank = (index/8) - 1, file = (index % 8) + 1; rank >= 0 && file <= 7; rank--, file++) {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    for (int rank = (index/8) - 1, file = (index % 8) - 1; rank >= 0 && file >= 0; rank--, file--) {
        attack_board |= one << (rank * 8 + file);
        if (one << (rank * 8 + file) & block_table) break;
    }
    return attack_board;
}