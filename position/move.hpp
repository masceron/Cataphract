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

constexpr int algebraic_to_num(const std::string_view algebraic)
{
    if (algebraic.length() != 2) return -1;
    int rank = 0;
    int file = 0;
    if (algebraic[1] >= '1' && algebraic[1] <= '8') rank = 8 - (algebraic[1] - '0');
    else return -1;
    switch (algebraic[0])
    {
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

constexpr const char* num_to_algebraic(const int sq) {
    static constexpr const char* table[64] = {
        "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
        "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
        "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
        "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
        "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
        "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
        "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
        "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
    };

    return table[sq];
}

struct Move
{
    uint16_t move;

    explicit Move(const uint16_t _from, const uint16_t _to, const uint16_t _flag) : move(
        _from | (_to << 6) | (_flag << 12))
    {
    }

    explicit Move(const uint16_t _move) : move(_move)
    {
    }

    Move() = default;

    [[nodiscard]] uint16_t flag() const
    {
        return move >> 12;
    }

    [[nodiscard]] uint16_t from() const
    {
        return move & 0b111111;
    }

    [[nodiscard]] uint16_t to() const
    {
        return move >> 6 & 0b111111;
    }

    [[nodiscard]] bool is_quiet() const
    {
        const auto _flag = flag();
        return !(_flag == capture || _flag == ep_capture || _flag == queen_promotion || _flag == queen_promo_capture);
    }

    [[nodiscard]] explicit operator bool() const
    {
        return move != 0;
    }

    [[nodiscard]] Pieces promoted_to(const bool side = white) const
    {
        if (side == white) return static_cast<Pieces>((flag() & 0b11) + 1);
        return static_cast<Pieces>(6 + ((flag() & 0b11) + 1));
    }

    bool operator==(const Move _move) const { return _move.move == this->move; }

    [[nodiscard]] std::string_view get_move_string() const
    {
        static char buffer[5];
        const auto from_sq = num_to_algebraic(from());
        const auto to_sq = num_to_algebraic(to());
        buffer[0] = from_sq[0];
        buffer[1] = from_sq[1];
        buffer[2] = to_sq[0];
        buffer[3] = to_sq[1];

        static auto promoted = "nbrqnbrq";

        if (flag() >= knight_promotion)
        {
            buffer[4] = promoted[flag() - 8];

            return {buffer, 5};
        }

        return {buffer, 4};
    }
};

inline const Move null_move(0);
