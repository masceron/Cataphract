#pragma once

#include <cstdint>

#include "position.hpp"
#include "../eval/eval.hpp"

inline uint64_t attackers_of(const Position& pos, const uint8_t sq, const uint64_t occ)
{
    const uint64_t target = 1ull << sq;
    const uint64_t diagonal_sliders = pos.boards[b] | pos.boards[B] | pos.boards[Q] | pos.boards[q];
    const uint64_t orthogonal_sliders = pos.boards[r] | pos.boards[R] | pos.boards[Q] | pos.boards[q];
    uint64_t attackers = 0;

    attackers |= (Shift<Upleft, black>(target) | Shift<Upright, black>(target)) & pos.boards[P];
    attackers |= (Shift<Upleft, white>(target) | Shift<Upright, white>(target)) & pos.boards[p];
    attackers |= knight_attack_tables[sq] & (pos.boards[N] | pos.boards[n]);
    attackers |= get_bishop_attack(sq, occ) & diagonal_sliders;
    attackers |= get_rook_attack(sq, occ) & orthogonal_sliders;
    attackers |= king_attack_tables[sq] & (pos.boards[K] | pos.boards[k]);

    return attackers;
}

inline int value_of(const Pieces piece)
{
    switch (piece) {
        case P: case p:
            return pawn_weight;
        case N: case n:
            return knight_weight;
        case B: case b:
            return bishop_weight;
        case R: case r:
            return rook_weight;
        case Q: case q:
            return queen_weight;
        default:
            return 0;
    }
}

inline uint64_t least_valuable_piece(const Position& pos, const uint64_t attackers, const uint8_t side, Pieces& attacker)
{
    for (int i = P + side * 6; i <= Q + side * 6; i++) {
        if (const uint64_t attack = attackers & pos.boards[i]) {
            attacker = static_cast<Pieces>(i);
            return attack & ~(attack - 1);
        }
    }
    return 0;
}

inline uint64_t get_x_ray(const Position& pos, const uint64_t from_set, const uint8_t to, const uint64_t occ)
{
    if (const uint8_t from = least_significant_one(from_set); from / 8 == to / 8 || from % 8 == to % 8) {
        const uint64_t orthogonal_sliders = (pos.boards[R] | pos.boards[r] | pos.boards[Q] | pos.boards[q]) ^ from_set;
        return get_rook_attack(to, occ) & orthogonal_sliders;
    }
    const uint64_t diagonal_sliders = (pos.boards[B] | pos.boards[b] | pos.boards[Q] | pos.boards[q]) ^ from_set;
    return get_bishop_attack(to, occ) & diagonal_sliders;
}

inline int16_t static_exchange_evaluation(const Position& pos, const Move& capture)
{
    bool side = pos.side_to_move;
    int8_t depth = 0;
    int gains[32];
    const uint8_t to = capture.dest();
    const uint8_t from = capture.src();
    uint64_t from_set = 1ull << from;
    const uint64_t x_ray_able = pos.boards[P] | pos.boards[p] | pos.boards[B] | pos.boards[b] | pos.boards[R] | pos.boards[r] | pos.boards[Q] | pos.boards[q];
    uint64_t occ = pos.occupations[2];
    uint64_t attackers = attackers_of(pos, to, occ);
    gains[depth] = value_of(pos.piece_on[to]);
    Pieces attacker = pos.piece_on[from];
    uint64_t attacked = 0;

    do {
        depth++;

        gains[depth] = value_of(attacker) - gains[depth - 1];
        attackers ^= from_set;
        occ ^= from_set;
        side = !side;
        attacked |= from_set;

        if (from_set & x_ray_able) {
            attackers |= ~attacked & get_x_ray(pos, from_set, to, occ);
        }
        from_set = least_valuable_piece(pos, attackers, side, attacker);

    } while (from_set);

    while (--depth) {
        gains[depth - 1] = -std::max(-gains[depth - 1], gains[depth]);
    }
    
    return gains[0];
}
