#pragma once

#include "move.hpp"
#include "position.hpp"
#include "../board/pieces/king.hpp"
#include "../board/pieces/knight.hpp"
#include "../board/pieces/pawn.hpp"
#include "../board/pieces/slider.hpp"

struct MoveList
{
    Move list[218];
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

inline void pawn_move_generator(Move*& pos, const MoveType type)
{
    const bool us = position.side_to_move;
    const uint8_t ep_square = position.state->en_passant_square;
    const bool enemy = 1 - us;
    const uint64_t board = position.boards[us == white ? P : p];
    const int8_t upleft_shift = us == white ? 9 : -9;
    const int8_t upright_shift = us == white ? 7 : -7;
    const int8_t upshift = us == white ? 8 : -8;
    const uint64_t last_line = us == white ? 0xFF00ull : 0xFF000000000000ull;
    const uint64_t line_3 = us == white ? 0xFF00000000ull: 0xFF000000ull;
    const uint64_t check_blocker = position.state->checker ? position.state->check_blocker : 0ull;

    uint64_t left_capture = (upleft_shift > 0 ? (((board & ~last_line) >> 9) & not_h_file) : (((board & ~last_line) << 9) & not_a_file)) & position.occupations[enemy];
    uint64_t right_capture = (upright_shift > 0 ? (((board & ~last_line) >> 7) & not_a_file) : (((board & ~last_line) << 7)) & not_h_file) & position.occupations[enemy];

    if (type == evasions) {
        left_capture &= check_blocker;
        right_capture &= check_blocker;
    }

    while (left_capture) {
        const uint8_t to = least_significant_one(left_capture);
        *(pos++) = Move(to + upleft_shift, to, capture);
        left_capture &= left_capture - 1;
    }

    while (right_capture) {
        const uint8_t to = least_significant_one(right_capture);
        *(pos++) = Move(to + upright_shift, to, capture);
        right_capture &= right_capture - 1;
    }

    if (position.state->en_passant_square != -1) {
        if (type != evasions || !(check_blocker & (1ull << (ep_square + (us == white ? -8 : 8))))) {
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

            uint64_t promote_capture = (((upleft_shift > 0 ? ((from_board >> 9) & not_h_file) : ((from_board << 9) & not_a_file))
                                    | (upright_shift > 0 ? ((from_board >> 7) & not_a_file) : ((from_board << 7) & not_h_file))) & position.occupations[enemy]);

            if (type == evasions) promote_capture &= check_blocker;
            while (promote_capture) {
                const uint8_t to = least_significant_one(promote_capture);
                *(pos++) = Move(from, to, queen_promo_capture);
                *(pos++) = Move(from, to, rook_promo_capture);
                *(pos++) = Move(from, to, bishop_promo_capture);
                *(pos++) = Move(from, to, knight_promo_capture);
                promote_capture &= promote_capture - 1;
            }

            if (type != captures) {
                uint64_t promote = (upshift > 0 ? (from_board >> 8) : (from_board << 8)) & (~position.occupations[2]);

                if (type == evasions) promote &= check_blocker;

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

    if (type != captures) {
        const uint64_t normal_pawns = board & (~last_line);
        uint64_t single_pawn_push = (upshift > 0 ? (normal_pawns >> 8) : (normal_pawns << 8)) & ~position.occupations[2];
        uint64_t double_pawn_push = (upshift > 0 ? (normal_pawns >> 16) : (normal_pawns << 16)) & line_3 & ~position.occupations[2];
        if (type == evasions) {
            single_pawn_push &= check_blocker;
            double_pawn_push &= check_blocker;
        }
        while (double_pawn_push) {
            if (const uint8_t to = least_significant_one(double_pawn_push); !((1ull << (to + upshift)) & position.occupations[2]))
                *(pos++) = Move(to + upshift + upshift, to, double_push);

            double_pawn_push &= double_pawn_push - 1;
        }
        while (single_pawn_push) {
            const uint8_t to = least_significant_one(single_pawn_push);
            *(pos++) = Move(to + upshift, to, quiet_move);

            single_pawn_push &= single_pawn_push - 1;
        }
    }
}

inline void general_move_generator(Move*& pos, const Pieces piece, const MoveType type)
{
    uint64_t board = 0;
    const bool us = position.side_to_move;
    const uint64_t occ = position.occupations[us];
    const uint64_t eocc = position.occupations[1 - us];
    const uint64_t aocc = position.occupations[2];
    switch (piece) {
        case Q:
            board = position.boards[us == white ? Q : q];
        break;
        case R:
            board = position.boards[us == white ? R : r];
        break;
        case B:
            board = position.boards[us == white ? B : b];
        break;
        case N:
            board = position.boards[us == white ? N : n];
        break;
        default:
            break;
    }
    while (board) {
        const uint8_t from = least_significant_one(board);
        uint64_t moveable = 0;
        switch (piece) {
            case Q:
                moveable = get_bishop_attack(from, aocc) | get_rook_attack(from, aocc);
            break;
            case R:
                moveable = get_rook_attack(from, aocc);
            break;
            case B:
                moveable = get_bishop_attack(from, aocc);
            break;
            case N:
                moveable = knight_attack_tables[from];
            break;
            default:
                break;
        }
        moveable &= type == evasions ? position.state->check_blocker : (~occ);

        uint64_t attacks = moveable & eocc;

        if (type != captures) {
            moveable = moveable & (~attacks);
            while (moveable) {
                *(pos++) = Move(from, least_significant_one(moveable), quiet_move);
                moveable &= moveable - 1;
            }
        }
        while (attacks) {
            *(pos++) = Move(from, least_significant_one(attacks), capture);
            attacks &= attacks - 1;
        }

        board &= board - 1;
    }
}

inline void king_move_generator(Move*& pos, const MoveType type, const uint8_t king)
{
    const bool us = position.side_to_move;
    const uint64_t occ = position.occupations[us];
    const uint64_t eocc = position.occupations[1 - us];
    const uint64_t aocc = position.occupations[2];

    uint64_t moveable = king_attack_tables[king] & (type == evasions ? ((~position.state->check_blocker & ~occ) | eocc) : ~occ);
    uint64_t attacks = moveable & eocc;

    if (type != captures) {
        moveable = moveable & ~attacks;
        while (moveable) {
            *(pos++) = Move(king, least_significant_one(moveable), quiet_move);
            moveable &= moveable - 1;
        }

        if (const uint8_t castling_rights = us == white ? (position.state->castling_rights & 0b11) : (position.state->castling_rights >> 2); type != evasions && castling_rights) {
            const uint64_t kp = us == white ? 0x6000000000000000ull : 0x60ull;
            const uint64_t qp = us == white ? 0xE00000000000000ull : 0xEull;
            if ((castling_rights & 0b01) && !(kp & aocc)) {
                *(pos++) = Move(king, king + 2, king_castle);
            }
            if ((castling_rights & 0b10) && !(qp & aocc)) {
                *(pos++) = Move(king, king - 2, queen_castle);
            }
        }

    }

    while (attacks) {
        *(pos++) = Move(king, least_significant_one(attacks) , capture);
        attacks &= attacks - 1;
    }
}


inline void pseudo_legal_move_generator(const MoveType type, Move*& last)
{
    if (std::popcount(position.state->checker) != 2) {
        pawn_move_generator(last, type);
        general_move_generator(last, Q, type);
        general_move_generator(last, R, type);
        general_move_generator(last, B, type);
        general_move_generator(last, N, type);
    }
    king_move_generator(last, type, least_significant_one(position.boards[position.side_to_move == white ? K : k]));

}

inline bool is_legal(const Move& move)
{
    const bool us = position.side_to_move;
    const bool enemy = 1 - position.side_to_move;
    const uint8_t from = move.src();
    const uint8_t to = move.dest();
    const uint8_t flag = move.flag();
    const uint64_t king_board = position.boards[us == white ? K : k];
    const uint8_t king = least_significant_one(king_board);

    if (flag == king_castle) {
        for (uint8_t k = from; k <= from + 2; k++) {
            if (position.is_square_attacked_by(k, enemy)) return false;
        }
    }
    else if (flag == queen_castle) {
        for (uint8_t k = from; k >= from - 2; k--) {
            if (position.is_square_attacked_by(k, enemy)) return false;
        }
    }
    else if (flag == ep_capture) {
        const uint64_t eq = position.boards[us == white ? q : Q];
        const uint64_t eb = position.boards[us == white ? b : B];
        const uint64_t er = position.boards[us == white ? r : R];
        const uint64_t occ = (position.occupations[2] ^ (1ull << (to + (us == white ? 8 : -8))) ^ (1ull << from)) | (1ull << to);

        return !((get_bishop_attack(king, occ) & (eq | eb)) | (get_rook_attack(king, occ) & (eq | er)));
    }
    else if (position.piece_on[from] == k || position.piece_on[from] == K) {
        position.occupations[2] ^= king_board;
        if (position.is_square_attacked_by(to, enemy)) {
            position.occupations[2] ^= king_board;
            return false;
        }
        position.occupations[2] ^= king_board;
    }

    return (!(position.state->pinned & (1ull << from)) || (lines_intersect[from][to] & king_board));
}


inline MoveList legal_move_generator()
{
    MoveList move_list;
    const bool us = position.side_to_move;
    if (position.state->checker)
        pseudo_legal_move_generator(evasions, move_list.last);
    else pseudo_legal_move_generator(non_evasions, move_list.last);

    const uint8_t king = us == white ? K : k;
    Move* current = move_list.begin();
    while (current != move_list.last) {
        if (Move move = *current; ((position.state->pinned & (1ull << move.src())) || (position.piece_on[move.src()] == king) || move.flag() == ep_capture) && (!is_legal(move))) {
            *current = *(--move_list.last);
        }
        else {
            current++;
        }
    }
    return move_list;
}

inline MoveList capture_move_generator()
{
    const bool us = position.side_to_move;
    MoveList move_list;
    pseudo_legal_move_generator(captures, move_list.last);
    const uint8_t king = us == white ? K : k;
    Move* current = move_list.begin();
    while (current != move_list.last) {
        if (Move move = *current;
            (position.state->checker && !((1ull << move.dest()) & position.state->check_blocker)) ||
            (((position.state->pinned & (1ull << move.src())) || (position.piece_on[move.src()] == king) || move.flag() == ep_capture) && (!is_legal(move)))) {
            *current = *(--move_list.last);
        }
        else {
            current++;
        }
    }
    return move_list;
}
