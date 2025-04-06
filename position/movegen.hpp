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
    MoveList(): list(), last(list) {}
};

enum MoveType: uint8_t
{
    evasions, non_evasions, captures
};

template<bool us, MoveType type>
void pawn_move_generator(const Position& cr_pos, Move*& pos)
{
    const uint8_t ep_square = cr_pos.state->en_passant_square;
    static constexpr bool enemy = 1 - us;
    static constexpr uint8_t board_index = us == white ? P : p;
    static constexpr uint64_t last_line = us == white ? 0xFF00ull : 0xFF000000000000ull;
    static constexpr uint64_t line_3 = us == white ? 0xFF00000000ull: 0xFF000000ull;

    const uint64_t board = cr_pos.boards[board_index];
    const uint64_t check_blocker = cr_pos.state->checker ? cr_pos.state->check_blocker : 0ull;

    uint64_t left_capture = Shift<Upleft, us>(board & ~last_line) & cr_pos.occupations[enemy];
    uint64_t right_capture = Shift<Upright, us>(board & ~last_line) & cr_pos.occupations[enemy];

    if constexpr (type == evasions) {
        left_capture &= check_blocker;
        right_capture &= check_blocker;
    }

    static constexpr int8_t upleft_shift = us == white ? -9 : 9;
    static constexpr int8_t upright_shift = us == white ? -7 : 7;

    while (left_capture) {
        const uint8_t to = least_significant_one(left_capture);
        *(pos++) = Move(to - upleft_shift, to, capture);
        left_capture &= left_capture - 1;
    }

    while (right_capture) {
        const uint8_t to = least_significant_one(right_capture);
        *(pos++) = Move(to - upright_shift, to, capture);
        right_capture &= right_capture - 1;
    }

    if (cr_pos.state->en_passant_square != -1) {
        if constexpr (type != evasions) {
            uint64_t eligible_pawns = pawn_attack_tables[enemy][ep_square] & board;
            while (eligible_pawns) {
                *(pos++) = Move(least_significant_one(eligible_pawns), ep_square, ep_capture);
                eligible_pawns &= eligible_pawns - 1;
            }
        }
        else if (!(check_blocker & Shift<Up, us>(1ull << ep_square))) {
            uint64_t eligible_pawns = pawn_attack_tables[enemy][ep_square] & board;
            while (eligible_pawns) {
                *(pos++) = Move(least_significant_one(eligible_pawns), ep_square, ep_capture);
                eligible_pawns &= eligible_pawns - 1;
            }
        }
    }

    if (uint64_t about_to_promote = board & last_line) {
        while (about_to_promote) {
            const uint8_t from = least_significant_one(about_to_promote);
            const uint64_t from_board = (1ull << from);

            uint64_t promote_capture = (Shift<Upleft, us>(from_board) | Shift<Upright, us>(from_board)) & cr_pos.occupations[enemy];

            if constexpr (type == evasions) promote_capture &= check_blocker;
            while (promote_capture) {
                const uint8_t to = least_significant_one(promote_capture);
                *(pos++) = Move(from, to, queen_promo_capture);
                *(pos++) = Move(from, to, rook_promo_capture);
                *(pos++) = Move(from, to, bishop_promo_capture);
                *(pos++) = Move(from, to, knight_promo_capture);
                promote_capture &= promote_capture - 1;
            }

            if constexpr (type != captures) {
                uint64_t promote = Shift<Up, us>(from_board) & (~cr_pos.occupations[2]);

                if constexpr (type == evasions) promote &= check_blocker;

                if (promote) {
                    const uint8_t to = least_significant_one(promote);
                    *(pos++) = Move(from, to, queen_promotion);
                    *(pos++) = Move(from, to, rook_promotion);
                    *(pos++) = Move(from, to, bishop_promotion);
                    *(pos++) = Move(from, to, knight_promotion);
                }
            }
            about_to_promote &= about_to_promote - 1;
        }
    }

    static constexpr int8_t upshift = us == white ? -8 : 8;

    if constexpr (type != captures) {
        const uint64_t normal_pawns = board & (~last_line);
        uint64_t single_pawn_push = Shift<Up,us>(normal_pawns) & ~cr_pos.occupations[2];
        uint64_t double_pawn_push = Shift<Up, us>(Shift<Up, us>(normal_pawns)) & line_3 & ~cr_pos.occupations[2];

        if constexpr (type == evasions) {
            single_pawn_push &= check_blocker;
            double_pawn_push &= check_blocker;
        }
        while (double_pawn_push) {
            if (const uint8_t to = least_significant_one(double_pawn_push); !(Shift<Up, enemy>(1ull << to) & cr_pos.occupations[2]))
                *(pos++) = Move(to - upshift - upshift, to, double_push);

            double_pawn_push &= double_pawn_push - 1;
        }
        while (single_pawn_push) {
            const uint8_t to = least_significant_one(single_pawn_push);
            *(pos++) = Move(to - upshift, to, quiet_move);

            single_pawn_push &= single_pawn_push - 1;
        }
    }
}

template<bool us, MoveType type, Pieces piece>
void general_move_generator(const Position& cr_pos, Move*& pos)
{
    uint64_t board = 0;
    const uint64_t occ = cr_pos.occupations[us];
    const uint64_t eocc = cr_pos.occupations[1 - us];
    const uint64_t aocc = cr_pos.occupations[2];
    constexpr uint8_t board_index = us == white ? (piece == Q ? Q : (piece == R ? R : (piece == B ? B : N)))
                                        : (piece == Q ? q : (piece == R ? r : (piece == B ? b : n)));

    board = cr_pos.boards[board_index];

    while (board) {
        const uint8_t from = least_significant_one(board);
        uint64_t movable = 0;
        if constexpr (piece == Q) {
            movable = get_bishop_attack(from, aocc) | get_rook_attack(from, aocc);
        }
        else if constexpr (piece == R) {
            movable = get_rook_attack(from, aocc);
        }
        else if constexpr (piece == B) {
            movable = get_bishop_attack(from, aocc);
        }
        else if constexpr (piece == N) {
            movable = knight_attack_tables[from];
        }
        
        if constexpr (type == evasions) movable &= cr_pos.state->check_blocker;
        else movable &= (~occ);

        uint64_t attacks = movable & eocc;

        if constexpr (type != captures) {
            movable = movable & (~attacks);
            while (movable) {
                *(pos++) = Move(from, least_significant_one(movable), quiet_move);
                movable &= movable - 1;
            }
        }
        while (attacks) {
            *(pos++) = Move(from, least_significant_one(attacks), capture);
            attacks &= attacks - 1;
        }

        board &= board - 1;
    }
}

template<bool us, MoveType type>
void king_move_generator(const Position& cr_pos, Move*& pos, const uint8_t king)
{
    const uint64_t occ = cr_pos.occupations[us];
    const uint64_t eocc = cr_pos.occupations[1 - us];
    const uint64_t aocc = cr_pos.occupations[2];

    uint64_t movable = king_attack_tables[king];
    if constexpr (type == evasions) movable &= (~cr_pos.state->check_blocker & ~occ) | eocc;
    else movable &= ~occ;

    uint64_t attacks = movable & eocc;

    if constexpr (type != captures) {
        movable = movable & ~attacks;
        while (movable) {
            *(pos++) = Move(king, least_significant_one(movable), quiet_move);
            movable &= movable - 1;
        }

        uint8_t castling_rights;
        
        if constexpr (us == white) castling_rights = cr_pos.state->castling_rights & 0b11;
        else castling_rights = cr_pos.state->castling_rights >> 2;

        if constexpr (type != evasions) {
            if (castling_rights) {
                static constexpr uint64_t kp = us == white ? 0x6000000000000000ull : 0x60ull;
                static constexpr uint64_t qp = us == white ? 0xE00000000000000ull : 0xEull;
                if ((castling_rights & 0b01) && !(kp & aocc)) {
                    *(pos++) = Move(king, king + 2, king_castle);
                }
                if ((castling_rights & 0b10) && !(qp & aocc)) {
                    *(pos++) = Move(king, king - 2, queen_castle);
                }
            }
        }

    }

    while (attacks) {
        *(pos++) = Move(king, least_significant_one(attacks) , capture);
        attacks &= attacks - 1;
    }
}

template<bool us, MoveType type>
void pseudo_legal_move_generator(const Position& cr_pos, Move*& last)
{
    if (std::popcount(cr_pos.state->checker) != 2) {
        pawn_move_generator<us, type>(cr_pos, last);
        general_move_generator<us, type, Q>(cr_pos, last);
        general_move_generator<us, type, R>(cr_pos, last);
        general_move_generator<us, type, B>(cr_pos, last);
        general_move_generator<us, type, N>(cr_pos, last);
    }
    constexpr uint8_t king = us == white ? K : k;
    king_move_generator<us, type>(cr_pos, last, least_significant_one(cr_pos.boards[king]));

}

inline MoveList legal_move_generator(Position& cr_pos)
{
    MoveList move_list;
    const bool us = cr_pos.side_to_move;
    if (cr_pos.state->checker) {
        if (us == white) pseudo_legal_move_generator<white, evasions>(cr_pos, move_list.last);
        else pseudo_legal_move_generator<black, evasions>(cr_pos, move_list.last);
    }
    else {
        if (us == white)pseudo_legal_move_generator<white, non_evasions>(cr_pos, move_list.last);
        else pseudo_legal_move_generator<black, non_evasions>(cr_pos, move_list.last);
    }

    const uint8_t king = us == white ? K : k;
    Move* current = move_list.begin();
    while (current != move_list.last) {
        if (Move move = *current; ((cr_pos.state->pinned & (1ull << move.src())) || (cr_pos.piece_on[move.src()] == king) || move.flag() == ep_capture) && (!cr_pos.is_legal(move))) {
            *current = *(--move_list.last);
        }
        else {
            current++;
        }
    }
    return move_list;
}

inline MoveList capture_move_generator(Position& cr_pos)
{
    const bool us = cr_pos.side_to_move;
    MoveList move_list;

    if (us == white) pseudo_legal_move_generator<white, captures>(cr_pos, move_list.last);
    else pseudo_legal_move_generator<black, captures>(cr_pos, move_list.last);

    const uint8_t king = us == white ? K : k;

    Move* current = move_list.begin();

    while (current != move_list.last) {
        if (Move move = *current;
            (cr_pos.state->checker && !((1ull << move.dest()) & cr_pos.state->check_blocker)) ||
            (((cr_pos.state->pinned & (1ull << move.src())) || (cr_pos.piece_on[move.src()] == king) || move.flag() == ep_capture) && (!cr_pos.is_legal(move)))) {
            *current = *(--move_list.last);
        }
        else {
            current++;
        }
    }
    return move_list;
}
