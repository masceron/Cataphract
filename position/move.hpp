#pragma once

#include <cstdint>
#include <string>

#include "../board/bitboard.hpp"

enum class MoveFlag: uint16_t
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

int algebraic_to_num(std::string_view algebraic);

const char* num_to_algebraic(int sq);

struct Move
{
    uint16_t move;

    explicit Move(uint16_t _from, uint16_t _to, MoveFlag _flag);

    explicit Move(uint16_t _move);

    Move() = default;

    [[nodiscard]] MoveFlag flag() const;
    [[nodiscard]] uint16_t from() const;
    [[nodiscard]] uint16_t to() const;
    [[nodiscard]] bool is_quiet() const;
    [[nodiscard]] explicit operator bool() const;
    [[nodiscard]] Piece promoted_to(bool side = white) const;
    [[nodiscard]] bool operator==(Move _move) const;
    [[nodiscard]] std::string_view get_move_string() const;
};

template <>
struct std::hash<Move>
{
    std::size_t operator()(const Move& m) const noexcept
    {
        return std::hash<uint16_t>{}(m.move);
    }
};

inline const Move null_move(0);
