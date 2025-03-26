#pragma once
#include <cstdint>
#include <array>
#include "../bitboard.h"

consteval uint64_t mask_king_attack(const uint8_t index) {
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
    for (uint8_t i = 0; i < 64; i ++) {
        king_tables[i] = mask_king_attack(i);
    }
    return king_tables;
}

constexpr std::array<uint64_t, 64> king_attack_tables = generate_king_attack_tables();