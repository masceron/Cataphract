#pragma once

#include <cstdint>
#include <deque>

#include "move.hpp"
#include "zobrist.hpp"
#include "../board/pieces/pawn.hpp"

inline Side color_of(const Pieces piece)
{
    if (piece < 6) return white;
    return black;
}

bool is_square_attacked_by(uint8_t index, bool side);

bool is_king_in_check_by(bool side);

uint64_t get_pinned_board_of(bool side);

uint64_t get_checker_of(bool side);

uint64_t get_check_blocker_of(bool side);

struct State
{
    uint16_t rule_50 = 0;
    uint8_t castling_rights = 0;
    int en_passant_square = -1;
    Pieces captured_piece = nil;
    State* previous = nullptr;
    uint64_t pawn_key = 0;
    uint64_t non_pawn_key = 0;
    uint64_t castling_key = 0;
    uint64_t en_passant_key = 0;
    uint64_t side_key = 0;
    uint8_t repetition = 1;

    uint64_t key = 0;
    uint16_t ply = 0;

};

inline std::deque<State> states(1);

class Position
{
public:
    Pieces piece_on[64]{};
    uint64_t boards[12]{};
    uint64_t occupations[3]{};
    uint64_t pinned[2]{};
    uint64_t checker = 0;
    bool side_to_move = white;
    State* state = nullptr;
    Position()
    {
        new_game();
    }

    void new_game()
    {
        states.clear();
        states.emplace_back();
        state = &states.back();
        std::fill_n(piece_on, 64, nil);
        std::fill_n(boards, 12, 0);
        std::fill_n(occupations, 3, 0);
        std::fill_n(pinned, 2, 0);
    }

    void move_piece(const uint8_t from, const uint8_t to)
    {
        const Pieces piece = piece_on[from];
        const uint64_t ft = (1ull << from) | (1ull << to);
        boards[piece] ^= ft;
        occupations[color_of(piece)] ^= ft;
        occupations[2] ^= ft;
        piece_on[to] = piece;
        piece_on[from] = nil;
    }

    void put_piece(const Pieces piece, const uint8_t sq)
    {
        piece_on[sq] = piece;
        const uint64_t board = 1ull << sq;
        occupations[color_of(piece)] |= board;
        occupations[2] |= board;
        boards[piece] |= board;
    }

    void remove_piece(const uint8_t sq)
    {
        const Pieces piece = piece_on[sq];
        const uint64_t board = 1ull << sq;
        piece_on[sq] = nil;
        occupations[color_of(piece)] ^= board;
        occupations[2] ^= board;
        boards[piece] ^= board;
    }

    void make_move(const uint16_t& move, State& st)
    {
        st = *state;
        st.previous = state;
        st.rule_50++;
        st.ply++;
        state = &st;

        const uint8_t from = Moves::src(move);
        const uint8_t to = Moves::dest(move);
        const uint8_t flag = Moves::flag(move);
        const Pieces moving_piece = piece_on[from];
        const Pieces captured_piece = flag == ep_capture ? (side_to_move == white ? p : P) : piece_on[to];

        state->castling_key ^= zobrist_keys.castling_keys[state->castling_rights];

        if (flag == king_castle) {
            const Zobrist_Non_Pawn_Key rook = side_to_move == white ? z_R : z_r;
            move_piece(to + 1, to - 1);
            state->castling_rights &= (side_to_move == white ? 0b1100 : 0b0011);

            state->non_pawn_key ^= zobrist_keys.non_pawn_keys[rook][to + 1] ^ zobrist_keys.non_pawn_keys[rook][to - 1];
        }
        else if (flag == queen_castle) {
            const Zobrist_Non_Pawn_Key rook = side_to_move == white ? z_R : z_r;
            move_piece(to - 2, to + 1);
            state->castling_rights &= (side_to_move == white ? 0b1100 : 0b0011);

            state->non_pawn_key ^= zobrist_keys.non_pawn_keys[rook][to - 2] ^ zobrist_keys.non_pawn_keys[rook][to + 1];
        }

        if (captured_piece != nil) {
            if (captured_piece == P || captured_piece == p) {
                if (flag == ep_capture) {
                    const uint8_t captured_square = to + (side_to_move == white ? 8 : -8);
                    remove_piece(captured_square);

                    state->pawn_key ^= zobrist_keys.pawn_keys[1 - side_to_move][captured_square - 8];
                }
                else {
                    state->pawn_key ^= zobrist_keys.pawn_keys[1 - side_to_move][to - 8];
                    remove_piece(to);
                }
            }
            else {
                if ((state->castling_rights & 0b1100) && side_to_move == white) {
                    if (to == 0) {
                        state->castling_rights &= ~black_queen;
                    }
                    else if (to == 7) {
                        state->castling_rights &= ~black_king;
                    }
                }
                else if ((state->castling_rights & 0b0011) && side_to_move == black) {
                    if (to == 56) {
                        state->castling_rights &= ~white_queen;
                    }
                    if (to == 63) {
                        state->castling_rights &= ~white_king;
                    }
                }

                remove_piece(to);

                state->non_pawn_key ^= zobrist_keys.non_pawn_keys[pieces_to_zobrist_key(captured_piece)][to];
            }
            state->rule_50 = 0;
        }

        if (state->en_passant_square != -1) {
            state->en_passant_key ^= zobrist_keys.en_passant_key[state->en_passant_square % 8];
            state->en_passant_square = -1;
        }

        move_piece(from, to);

        if (moving_piece == P || moving_piece == p) {
            if (flag == double_push &&
                (pawn_attack_tables[side_to_move][to + (side_to_move == white ? 8 : -8)] & boards[side_to_move == white ? p : P])) {
                state->en_passant_square = to + (side_to_move == white ? 8 : -8);
                state->en_passant_key ^= zobrist_keys.en_passant_key[state->en_passant_square % 8];
                state->pawn_key ^= zobrist_keys.pawn_keys[side_to_move][from - 8] ^ zobrist_keys.pawn_keys[side_to_move][to - 8];
            }
            else if (flag >= knight_promotion) {
                const Pieces promoted_to = Moves::promoted_to(move, side_to_move);
                remove_piece(to);
                put_piece(promoted_to, to);

                state->non_pawn_key ^= zobrist_keys.non_pawn_keys[pieces_to_zobrist_key(promoted_to)][to];
                state->pawn_key ^= zobrist_keys.pawn_keys[side_to_move][from - 8];
            }



            state->rule_50 = 0;
        }
        else {
            const Zobrist_Non_Pawn_Key key = pieces_to_zobrist_key(moving_piece);
            state->non_pawn_key ^= zobrist_keys.non_pawn_keys[key][from]
                                ^ zobrist_keys.non_pawn_keys[key][to];
        }

        if (moving_piece == K && from == 60 && (state->castling_rights & 0b11)) {
            state->castling_rights &= 0b1100;
        }
        else if (moving_piece == k && from == 4 && (state->castling_rights & 0b1100)) {
            state->castling_rights &= 0b0011;
        }
        else if (moving_piece == r) {
            if (from == 7 && (state->castling_rights & 0b0100)) state->castling_rights &= 0b1011;
            if (from == 0 && (state->castling_rights & 0b1000)) state->castling_rights &= 0b0111;
        }
        else if (moving_piece == R) {

            if (from == 63 && (state->castling_rights & 0b0001)) state->castling_rights &= 0b1110;
            if (from == 56 && (state->castling_rights & 0b0010)) state->castling_rights &= 0b1101;
        }

        state->castling_key ^= zobrist_keys.castling_keys[state->castling_rights];

        state->captured_piece = captured_piece;

        state->side_key ^= zobrist_keys.side_key;

        state->key = state->pawn_key ^ state->non_pawn_key ^ state->castling_key ^ state->en_passant_key ^ state->side_key;

        if (is_king_in_check_by(side_to_move)) {
            checker = get_checker_of(1 - side_to_move);
        }
        else checker = 0;

        side_to_move = 1 - side_to_move;

        pinned[side_to_move] = get_pinned_board_of(side_to_move);

        state->repetition = 1;
        if (const uint16_t cutoff = std::min(state->rule_50, state->ply); cutoff >= 4) {
            const State* tst = state->previous->previous;
            for (int cr = 4; cr <= cutoff; cr += 2) {
                tst = tst->previous->previous;
                if (state->key == tst->key) {
                    state->repetition = tst->repetition + 1;
                    break;
                }
            }
        }
    }

    void unmake_move(const uint16_t& move)
    {
        side_to_move = 1 - side_to_move;
        const uint8_t from = Moves::src(move);
        const uint8_t to = Moves::dest(move);
        const uint8_t flag = Moves::flag(move);

        if (flag >= knight_promotion) {
            remove_piece(to);
            put_piece(side_to_move == white ? P : p, to);
        }
        else if (flag == king_castle) {
            move_piece(to - 1, to + 1);
        }
        else if (flag == queen_castle) {
            move_piece(to + 1, to - 2);
        }

        move_piece(to, from);
        if (state->captured_piece != nil) {
            uint8_t captured_square = to;
            if (flag == ep_capture) {
                captured_square += side_to_move == white ? 8 : -8;
            }

            put_piece(state->captured_piece, captured_square);
        }

        state = state->previous;
        if (is_king_in_check_by(1 - side_to_move)) checker = get_checker_of(side_to_move);
    }

    void do_move(const uint16_t& move)
    {
        State st;
        make_move(move, st);
        states.push_back(st);
        state = &states.back();
    }
};

inline Position position;

inline void print_board()
{
    std::string lines[8];
    for (int rank = 0; rank < 8; rank++) {
        lines[rank] += std::to_string(8-rank) + "  ";
        for (int file = 0; file < 8; file++) {
            lines[rank] += ". ";
        }
        lines[rank] += "\n";
    }
    for (int pos = 0; pos < 64; pos++) {
        lines[pos / 8][3 + (pos % 8) * 2] = piece_icons[position.piece_on[pos]];
    }

    std::cout << "   a b c d e f g h\n";
    for (const auto& i: lines) std::cout << i;
    std::cout << "\n";
}