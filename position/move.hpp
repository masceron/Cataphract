#pragma once
#include <cstdint>

#include "../board/bitboard.hpp"
#include "fen.hpp"

enum flag: uint16_t
{
    quiet_move,
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

namespace Moves
{
    inline uint16_t Move(const uint16_t from, const uint16_t to, const uint16_t flag) {return from + (to << 6) + (flag << 12);}
    inline uint16_t src(const uint16_t& move) { return move & 0b111111; }
    inline uint16_t dest(const uint16_t& move) { return (move >> 6 & 0b111111); }
    inline uint16_t flag(const uint16_t& move) { return (move >> 12); }
    inline bool is_capture(const uint16_t& move) { return flag(move) & capture & ep_capture & bishop_promo_capture & rook_promo_capture & knight_promo_capture & queen_promo_capture; }
    inline Pieces promoted_to(const uint16_t& move, const bool side) { return static_cast<Pieces>(6 * side + ((flag(move) & 0b11) + 1)); }

}

inline std::string get_move_string(const uint16_t& move)
{
    std::stringstream s;
    s << num_to_algebraic(Moves::src(move)) << num_to_algebraic(Moves::dest(move));

    if (Moves::flag(move) >= knight_promotion) {
        switch (Moves::promoted_to(move, white)) {
            case N:
                s << "n";
            break;
            case B:
                s << "b";
            break;
            case R:
                s << "r";
            break;
            case Q:
                s << "q";
            break;
            default:
                break;
        }
    }
    return s.str();
}