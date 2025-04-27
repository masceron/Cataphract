#pragma once

#include <cstdint>

#include "../board/bitboard.hpp"

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

inline int algebraic_to_num(const std::string &algebraic)
{
    if (algebraic.length() != 2) return -1;
    int rank = 0;
    int file = 0;
    if (algebraic[1] >= '1' && algebraic[1] <= '8') rank = 8 - (algebraic[1] - '0');
    else return - 1;
    switch (algebraic[0]) {
        case 'a':
            file = 0;
        break;
        case 'b':
            file = 1;
        break;
        case 'c':
            file = 2;
        break;
        case 'd':
            file = 3;
        break;
        case 'e':
            file = 4;
        break;
        case 'f':
            file = 5;
        break;
        case 'g':
            file = 6;
        break;
        case 'h':
            file = 7;
        break;
        default:
            return -1;
    }
    return rank * 8 + file;
}

inline std::string num_to_algebraic(const uint8_t sq)
{
    static constexpr char files[] = "abcdefgh";

    return files[sq % 8] + std::to_string(8 - sq/8);
}

struct Move
{
    uint16_t move;
    explicit Move(const uint16_t src, const uint16_t dest, const uint16_t flags) : move(src | (dest << 6) | (flags << 12)) {}
    explicit Move(const uint16_t _move): move(_move) {}
    Move() {}
    [[nodiscard]] uint16_t flag() const { return move >> 12; }
    [[nodiscard]] uint16_t src() const { return move & 0b111111; }
    [[nodiscard]] uint16_t dest() const { return (move >> 6) & 0b111111; }
    template<const bool side_needed> [[nodiscard]] Pieces promoted_to(const bool side = white) const
    {
        if constexpr (!side_needed) return static_cast<Pieces>((flag() & 0b11) + 1);
        else {
            if (side == white) return static_cast<Pieces>((flag() & 0b11) + 1);
            return static_cast<Pieces>(6 + ((flag() & 0b11) + 1));
        }
    }

    bool operator==(const Move & _move) const { return _move.move == this->move; };
};

const Move move_none(0, 0, 0);

inline std::string get_move_string(const Move& move)
{
    if (move == move_none) return "0000";
    std::stringstream s;
    s << num_to_algebraic(move.src()) << num_to_algebraic(move.dest());

    if (move.flag() >= knight_promotion) {
        switch (move.promoted_to<false>()) {
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