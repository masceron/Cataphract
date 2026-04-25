#pragma once

#include "move.hpp"
#include "position.hpp"
#include "../board/pieces/king.hpp"
#include "../board/pieces/knight.hpp"
#include "../board/pieces/pawn.hpp"
#include "../board/pieces/slider.hpp"

struct MoveList
{
    Move list[256];
    Move* last;
    [[nodiscard]] Move* begin() { return list; }
    [[nodiscard]] Move* end() const { return last; }
    [[nodiscard]] int64_t size() const { return last - list; }
    Move operator[](const int index) const { return list[index]; }

    MoveList() : last(list)
    {
    }

    void reset()
    {
        last = list;
    }
};

enum MoveType: uint8_t
{
    noisy, quiet, all
};

template <const bool us, const MoveType type, const bool evasive>
void pawn_move_generator(const Position& cr_pos, Move*& pos)
{
    const auto ep_square = cr_pos.state->en_passant_square;
    static constexpr bool enemy = !us;
    static constexpr int board_index = us == white ? P : p;
    static constexpr uint64_t last_line = us == white ? 0xFF00ull : 0xFF000000000000ull;
    static constexpr uint64_t line_3 = us == white ? 0xFF00000000ull : 0xFF000000ull;

    const uint64_t board = cr_pos.boards[board_index];
    const uint64_t check_blocker = cr_pos.state->checker ? cr_pos.state->check_blocker : 0ull;

    static constexpr int8_t upleft_shift = us == white ? -9 : 9;
    static constexpr int8_t upright_shift = us == white ? -7 : 7;

    if constexpr (type != quiet)
    {
        uint64_t left_capture = Shift<Upleft, us>(board & ~last_line) & cr_pos.occupations[enemy];
        uint64_t right_capture = Shift<Upright, us>(board & ~last_line) & cr_pos.occupations[enemy];

        if constexpr (evasive)
        {
            left_capture &= check_blocker;
            right_capture &= check_blocker;
        }

        while (left_capture)
        {
            const int to = pop_lsb(left_capture);
            *pos++ = Move(to - upleft_shift, to, capture);
        }

        while (right_capture)
        {
            const int to = pop_lsb(right_capture);
            *pos++ = Move(to - upright_shift, to, capture);
        }
    }

    if constexpr (type != quiet)
    {
        if (cr_pos.state->en_passant_square != -1)
        {
            if constexpr (!evasive)
            {
                uint64_t eligible_pawns = pawn_attack_tables[enemy][ep_square] & board;
                while (eligible_pawns)
                {
                    *pos++ = Move(pop_lsb(eligible_pawns), ep_square, ep_capture);
                }
            }
            else if (!(check_blocker & Shift<Up, us>(1ull << ep_square)))
            {
                uint64_t eligible_pawns = pawn_attack_tables[enemy][ep_square] & board;
                while (eligible_pawns)
                {
                    *pos++ = Move(pop_lsb(eligible_pawns), ep_square, ep_capture);
                }
            }
        }
    }

    static constexpr int upshift = us == white ? -8 : 8;
    if (const uint64_t promoting_pawns = board & last_line)
    {
        if constexpr (type != quiet)
        {
            uint64_t left_promotions = Shift<Upleft, us>(promoting_pawns) & cr_pos.occupations[enemy];
            uint64_t right_promotions = Shift<Upright, us>(promoting_pawns) & cr_pos.occupations[enemy];

            if constexpr (evasive)
            {
                left_promotions &= check_blocker;
                right_promotions &= check_blocker;
            }

            while (left_promotions)
            {
                const int to = pop_lsb(left_promotions);
                const int from = to - upleft_shift;

                *pos++ = Move(from, to, queen_promo_capture);
                *pos++ = Move(from, to, rook_promo_capture);
                *pos++ = Move(from, to, bishop_promo_capture);
                *pos++ = Move(from, to, knight_promo_capture);
            }

            while (right_promotions)
            {
                const int to = pop_lsb(right_promotions);
                const int from = to - upright_shift;

                *pos++ = Move(from, to, queen_promo_capture);
                *pos++ = Move(from, to, rook_promo_capture);
                *pos++ = Move(from, to, bishop_promo_capture);
                *pos++ = Move(from, to, knight_promo_capture);
            }
        }

        uint64_t push_promotions = Shift<Up, us>(promoting_pawns) & ~cr_pos.occupations[2];
        if constexpr (evasive) push_promotions &= check_blocker;

        while (push_promotions)
        {
            const int to = pop_lsb(push_promotions);
            const int from = to - upshift;

            *pos++ = Move(from, to, queen_promotion);
            if constexpr (type != noisy)
            {
                *pos++ = Move(from, to, rook_promotion);
                *pos++ = Move(from, to, bishop_promotion);
                *pos++ = Move(from, to, knight_promotion);
            }
        }
    }

    if constexpr (type != noisy)
    {
        const uint64_t normal_pawns = board & ~last_line;
        uint64_t single_pawn_push = Shift<Up, us>(normal_pawns) & ~cr_pos.occupations[2];
        uint64_t double_pawn_push = Shift<Up, us>(single_pawn_push) & line_3 & ~cr_pos.occupations[2];

        if constexpr (evasive)
        {
            single_pawn_push &= check_blocker;
            double_pawn_push &= check_blocker;
        }
        while (double_pawn_push)
        {
            const int to = pop_lsb(double_pawn_push);
            *pos++ = Move(to - upshift - upshift, to, double_push);
        }
        while (single_pawn_push)
        {
            const int to = pop_lsb(single_pawn_push);
            *pos++ = Move(to - upshift, to, quiet_move);
        }
    }
}

template <const bool us, const MoveType type, const Pieces piece, const bool evasive>
void general_move_generator(const Position& cr_pos, Move*& pos)
{
    uint64_t board = 0;
    const uint64_t occ = cr_pos.occupations[us];
    const uint64_t eocc = cr_pos.occupations[!us];
    const uint64_t aocc = cr_pos.occupations[2];
    constexpr int board_index = us == white
                                    ? (piece == Q ? Q : piece == R ? R : piece == B ? B : N)
                                    : piece == Q
                                    ? q
                                    : piece == R
                                    ? r
                                    : piece == B
                                    ? b
                                    : n;

    constexpr int king = us == white ? K : k;
    const int king_sq = lsb(cr_pos.boards[king]);
    board = cr_pos.boards[board_index];

    while (board)
    {
        const int from = pop_lsb(board);
        uint64_t movable = 0;
        if constexpr (piece == Q)
        {
            movable = get_bishop_attack(from, aocc) | get_rook_attack(from, aocc);
        }
        else if constexpr (piece == R)
        {
            movable = get_rook_attack(from, aocc);
        }
        else if constexpr (piece == B)
        {
            movable = get_bishop_attack(from, aocc);
        }
        else if constexpr (piece == N)
        {
            movable = knight_attack_tables[from];
        }

        if (cr_pos.state->pinned & (1ull << from))
        {
            if constexpr (piece == N) continue;
            movable &= lines_intersect[king_sq][from];
        }

        if constexpr (evasive) movable &= cr_pos.state->check_blocker;
        else movable &= ~occ;

        uint64_t attacks = movable & eocc;

        if constexpr (type != noisy)
        {
            movable = movable & ~attacks;
            while (movable)
            {
                *pos++ = Move(from, pop_lsb(movable), quiet_move);
            }
        }

        if constexpr (type != quiet)
            while (attacks)
            {
                *pos++ = Move(from, pop_lsb(attacks), capture);
            }
    }
}

template <const bool us, const MoveType type, const bool evasive>
void king_move_generator(const Position& cr_pos, Move*& pos, const int king)
{
    const uint64_t occ = cr_pos.occupations[us];
    const uint64_t eocc = cr_pos.occupations[!us];
    const uint64_t aocc = cr_pos.occupations[2];

    const uint64_t enemy_threats = cr_pos.state->attacks;
    uint64_t movable = king_attack_tables[king] & ~enemy_threats & ~occ;

    uint64_t attacks = movable & eocc;

    if constexpr (type != noisy)
    {
        movable = movable & ~attacks;
        while (movable)
        {
            *pos++ = Move(king, pop_lsb(movable), quiet_move);
        }

        const uint8_t castling_rights = cr_pos.state->castling_rights;

        if constexpr (!evasive)
        {
            if (castling_rights)
            {
                static constexpr uint64_t king_path = us == white ? 0x6000000000000000ull : 0x60ull;
                static constexpr uint64_t queen_path = us == white ? 0xE00000000000000ull : 0xEull;
                static constexpr uint64_t queen_check_path = us == white ? 0xC00000000000000ull : 0xCull;

                if constexpr (us == white)
                {
                    if (castling_rights & 0b0001)
                    {
                        if (!(king_path & aocc) && !(king_path & enemy_threats))
                        {
                            *pos++ = Move(king, king + 2, king_castle);
                        }
                    }
                    if (castling_rights & 0b0010)
                    {
                        if (!(queen_path & aocc) && !(queen_check_path & enemy_threats))
                        {
                            *pos++ = Move(king, king - 2, queen_castle);
                        }
                    }
                }
                else
                {
                    if (castling_rights & 0b0100)
                    {
                        if (!(king_path & aocc) && !(king_path & enemy_threats))
                        {
                            *pos++ = Move(king, king + 2, king_castle);
                        }
                    }
                    if (castling_rights & 0b1000)
                    {
                        if (!(queen_path & aocc) && !(queen_check_path & enemy_threats))
                        {
                            *pos++ = Move(king, king - 2, queen_castle);
                        }
                    }
                }
            }
        }
    }

    if constexpr (type != quiet)
    {
        while (attacks)
        {
            *pos++ = Move(king, pop_lsb(attacks), capture);
        }
    }
}

template <bool us, MoveType type, const bool evasive>
void move_generator(const Position& cr_pos, Move*& last)
{
    if (std::popcount(cr_pos.state->checker) != 2)
    {
        pawn_move_generator<us, type, evasive>(cr_pos, last);
        general_move_generator<us, type, N, evasive>(cr_pos, last);
        general_move_generator<us, type, B, evasive>(cr_pos, last);
        general_move_generator<us, type, R, evasive>(cr_pos, last);
        general_move_generator<us, type, Q, evasive>(cr_pos, last);
    }
    static constexpr int king = us == white ? K : k;
    king_move_generator<us, type, evasive>(cr_pos, last, lsb(cr_pos.boards[king]));
}

template <const MoveType type>
void pseudo_legals(const Position& cr_pos, MoveList& list)
{
    const bool us = cr_pos.side_to_move;

    if (cr_pos.state->checker)
    {
        if (us == white) move_generator<white, type, true>(cr_pos, list.last);
        else move_generator<black, type, true>(cr_pos, list.last);
    }
    else
    {
        if (us == white) move_generator<white, type, false>(cr_pos, list.last);
        else move_generator<black, type, false>(cr_pos, list.last);
    }
}

inline bool check_move_legality(const Position& cr_pos, const Move move)
{
    if (const int from = move.from(); cr_pos.state->pinned & (1ull << from))
    {
        if (const auto king_board = cr_pos.boards[cr_pos.side_to_move == white ? K : k];
            !(lines_intersect[from][move.to()] & king_board))
        {
            return false;
        }
    }

    if (move.flag() == ep_capture && !cr_pos.is_legal(move))
    {
        return false;
    }
    return true;
}

template <const MoveType type>
void legals(const Position& cr_pos, MoveList& list)
{
    cr_pos.get_attacks();
    cr_pos.get_pinned();

    pseudo_legals<type>(cr_pos, list);
    Move* current = list.begin();

    while (current != list.last)
    {
        if (!check_move_legality(cr_pos, *current))
        {
            *current = *--list.last;
        }
        else
        {
            current++;
        }
    }
}
