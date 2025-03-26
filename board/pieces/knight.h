#pragma once
#include <cstdint>
#include <array>
#include "../bitboard.h"

consteval uint64_t mask_knight_attack(const uint8_t index) {
    uint64_t attack_board = 0;
    const uint64_t piece_board = (attack_board + 1) << index;
    attack_board |= (piece_board >> 17) & 0x7fffffffffff & not_h_file;
    attack_board |= (piece_board >> 15) & 0x1ffffffffffff & not_a_file;
    attack_board |= (piece_board >> 10) & 0x3fffffffffffff & not_gh_file;
    attack_board |= (piece_board >> 6)  & 0x3ffffffffffffff & not_ab_file;
    attack_board |= (piece_board << 17) & not_a_file;
    attack_board |= (piece_board << 15) & not_h_file;
    attack_board |= (piece_board << 10) & not_ab_file;
    attack_board |= (piece_board << 6)  & not_gh_file;
    return attack_board;
}

consteval std::array<uint64_t, 64> generate_knight_attack_tables()
{
    std::array<uint64_t, 64> knight_tables{};
    for (uint8_t i = 0; i < 64; i ++) {
        knight_tables[i] = mask_knight_attack(i);
    }
    return knight_tables;
}

constexpr std::array<uint64_t, 64> knight_attack_tables = generate_knight_attack_tables();