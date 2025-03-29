#pragma once
#include <cstdint>
#include "../bitboard.hpp"
#include <array>

constexpr uint64_t mask_pawn_attack(const uint8_t side, const uint8_t index)
{
    uint64_t attack_board = 0;
    const uint64_t piece_board = (attack_board + 1) << index;
    switch (side) {
        case white:
            attack_board |= (piece_board >> 7) & 0x1ffffffffffffff & not_a_file;
            attack_board |= (piece_board >> 9) & 0x7fffffffffffff & not_h_file;
            break;
        case black:
            attack_board |= (piece_board << 7) & not_h_file;
            attack_board |= (piece_board << 9) & not_a_file;
            break;
    }
    return attack_board;
}

constexpr std::array<std::array<uint64_t, 64>, 2> generate_pawn_attack_tables() {
    std::array<std::array<uint64_t, 64>, 2> pawn_attacks{};
    for (uint8_t i = 0; i < 64; i++) {
        pawn_attacks[0][i] = mask_pawn_attack(white, i);
        pawn_attacks[1][i] = mask_pawn_attack(black, i);
    }
    return pawn_attacks;
}

constexpr std::array<std::array<uint64_t, 64>, 2> pawn_attack_tables = generate_pawn_attack_tables();