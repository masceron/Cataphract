#pragma once
#include <cstdint>
#include "../board/bitboard.hpp"
enum flag: uint16_t
{
    quiet_moves,
    double_push,
    king_castle,
    queen_castle,
    capture,
    ep_capture,
    knight_promotion = 8,
    bishop_promotion,
    rook_promotion,
    queen_promotion,
    knight_promo_capture,
    bishop_promo_capture,
    rook_promo_capture,
    queen_promo_capture
};

class Move
{
public:
    uint16_t move;
    explicit Move(const uint16_t src, const uint16_t dest, const uint16_t flags)
    {
        move = src | (dest << 6) | (flags << 12);
    }
    [[nodiscard]] uint16_t flag() const { return move >> 12; }
    [[nodiscard]] uint16_t src() const { return move & 0b111111; }
    [[nodiscard]] uint16_t dest() const { return (move >> 6) & 0b111111; }
    [[nodiscard]] bool is_capture() const { return flag() & capture & ep_capture & bishop_promo_capture & rook_promo_capture & knight_promo_capture & queen_promo_capture; }
    [[nodiscard]] uint16_t promoted_to(const side side) const { return 6 * side + ((flag() & 0b11) + 1); }
};

class MoveList
{
public:
    Move move[218];
};