#pragma once

#include <cstdint>
#include <deque>
#include <cstring>
#include <bit>

#include "move.hpp"
#include "zobrist.hpp"
#include "../board/pieces/pawn.hpp"
#include "../board/pieces/slider.hpp"
#include "../board/pieces/knight.hpp"
#include "../board/pieces/king.hpp"
#include "../board/lines.hpp"
#include "../board/bitboard.hpp"

inline Side color_of(const Pieces piece)
{
    if (piece < 6) return white;
    return black;
}

struct State
{
    uint16_t rule_50;
    uint8_t castling_rights;
    int en_passant_square;
    uint64_t pawn_key;
    uint64_t non_pawn_key;
    uint64_t castling_key;
    uint64_t en_passant_key;
    uint64_t side_key;
    uint16_t ply;
    uint8_t ply_from_root;

    uint64_t key = 0;
    Pieces captured_piece;
    uint64_t pinned;
    uint64_t checker;
    uint64_t check_blocker;
    State* previous;
    uint8_t repetition;
};

inline std::deque<State> states(1);

struct Position
{
    Pieces piece_on[64]{};
    uint64_t boards[12]{};
    uint64_t occupations[3]{};
    bool side_to_move = white;
    State* state = nullptr;
    Position()
    {
        new_game();
    }

    void new_game()
    {
        std::fill_n(piece_on, 64, nil);
        std::fill_n(boards, 12, 0);
        std::fill_n(occupations, 3, 0);
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

    void make_move(const Move& move, State& st)
    {
        memcpy(&st, state, offsetof(State, key));
        st.previous = state;
        st.rule_50++;
        st.ply++;
        st.ply_from_root++;
        state = &st;

        const uint8_t from = move.src();
        const uint8_t to = move.dest();
        const uint8_t flag = move.flag();
        const Pieces moving_piece = piece_on[from];
        const Pieces captured_piece = flag == ep_capture ? (side_to_move == white ? p : P) : piece_on[to];

        if (flag == king_castle) {
            const Zobrist_Non_Pawn_Key rook = side_to_move == white ? z_R : z_r;
            move_piece(to + 1, to - 1);

            st.non_pawn_key ^= Zobrist::non_pawn_keys[rook][to + 1] ^ Zobrist::non_pawn_keys[rook][to - 1];
        }
        else if (flag == queen_castle) {
            const Zobrist_Non_Pawn_Key rook = side_to_move == white ? z_R : z_r;
            move_piece(to - 2, to + 1);

            st.non_pawn_key ^= Zobrist::non_pawn_keys[rook][to - 2] ^ Zobrist::non_pawn_keys[rook][to + 1];
        }

        else if (captured_piece != nil) {
            if (captured_piece == P || captured_piece == p) {
                if (flag == ep_capture) {
                    const uint8_t captured_square = to + (side_to_move == white ? 8 : -8);
                    remove_piece(captured_square);

                    st.pawn_key ^= Zobrist::pawn_keys[1 - side_to_move][captured_square - 8];
                }
                else {
                    st.pawn_key ^= Zobrist::pawn_keys[1 - side_to_move][to - 8];
                    remove_piece(to);
                }
            }
            else {
                if (to == 0 && side_to_move == white) {
                    st.castling_rights &= ~black_queen;
                    st.castling_key =  Zobrist::castling_keys[st.castling_rights];
                }
                else if (to == 7 && side_to_move == white) {
                    st.castling_rights &= ~black_king;
                    st.castling_key = Zobrist::castling_keys[st.castling_rights];
                }
                else if (to == 56 && side_to_move == black) {
                    st.castling_rights &= ~white_queen;
                    st.castling_key = Zobrist::castling_keys[st.castling_rights];
                }
                else if (to == 63 && side_to_move == black) {
                    st.castling_rights &= ~white_king;
                    st.castling_key = Zobrist::castling_keys[st.castling_rights];
                }

                remove_piece(to);

                st.non_pawn_key ^= Zobrist::non_pawn_keys[pieces_to_zobrist_key(captured_piece)][to];
            }
            st.rule_50 = 0;
        }

        if (st.en_passant_square != -1) {
            st.en_passant_key ^= Zobrist::en_passant_key[st.en_passant_square % 8];
            st.en_passant_square = -1;
        }

        move_piece(from, to);

        if (moving_piece == P || moving_piece == p) {
            if (flag == double_push &&
                (pawn_attack_tables[side_to_move][to + (side_to_move == white ? 8 : -8)] & boards[side_to_move == white ? p : P])) {
                st.en_passant_square = to + (side_to_move == white ? 8 : -8);
                st.en_passant_key ^= Zobrist::en_passant_key[st.en_passant_square % 8];
                st.pawn_key ^= Zobrist::pawn_keys[side_to_move][from - 8] ^ Zobrist::pawn_keys[side_to_move][to - 8];
            }
            else if (flag >= knight_promotion) {
                const Pieces promoted_to = move.promoted_to(side_to_move);
                remove_piece(to);
                put_piece(promoted_to, to);

                st.non_pawn_key ^= Zobrist::non_pawn_keys[pieces_to_zobrist_key(promoted_to)][to];
                st.pawn_key ^= Zobrist::pawn_keys[side_to_move][from - 8];
            }

            st.rule_50 = 0;
        }
        else {
            const Zobrist_Non_Pawn_Key key = pieces_to_zobrist_key(moving_piece);
            st.non_pawn_key ^= Zobrist::non_pawn_keys[key][from]
                                ^ Zobrist::non_pawn_keys[key][to];

            if (from == 60 && moving_piece == K) {
                st.castling_rights &= 0b1100;
                st.castling_key = Zobrist::castling_keys[st.castling_rights];
            }
            else if (from == 4 && moving_piece == k) {
                st.castling_rights &= 0b0011;
                st.castling_key = Zobrist::castling_keys[st.castling_rights];
            }
            else if (from == 7 && moving_piece == r) {
                st.castling_rights &= ~black_king;
                st.castling_key = Zobrist::castling_keys[st.castling_rights];
            }
            else if (from == 0 && moving_piece == r) {
                st.castling_rights &= ~black_queen;
                st.castling_key = Zobrist::castling_keys[st.castling_rights];
            }
            else if (from == 56 && moving_piece == R) {
                st.castling_rights &= ~white_queen;
                st.castling_key = Zobrist::castling_keys[st.castling_rights];
            }
            else if (from == 63 && moving_piece == R) {
                st.castling_rights &= ~white_king;
                st.castling_key = Zobrist::castling_keys[st.castling_rights];
            }
        }

        st.captured_piece = captured_piece;

        st.side_key ^= Zobrist::side_key;

        st.key = st.pawn_key ^ st.non_pawn_key ^ st.castling_key ^ st.en_passant_key ^ st.side_key;

        st.checker = get_checker_of(1 - side_to_move);

        if (st.checker) {
            st.check_blocker = get_check_blocker_of(1 - side_to_move);
        }

        side_to_move = 1 - side_to_move;

        st.pinned = get_pinned_board_of(side_to_move);

        st.repetition = 1;
        if (const uint16_t cutoff = std::min(st.rule_50, st.ply); cutoff >= 4) {
            const State* tst = st.previous->previous;
            for (int cr = 4; cr <= cutoff; cr += 2) {
                tst = tst->previous->previous;
                if (st.key == tst->key) {
                    st.repetition = tst->repetition + 1;
                    break;
                }
            }
        }
    }

    void unmake_move(const Move &move)
    {
        side_to_move = 1 - side_to_move;
        const uint8_t from = move.src();
        const uint8_t to = move.dest();
        const uint8_t flag = move.flag();

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
    }

    void do_move(const Move& move)
    {
        State st;
        make_move(move, st);
        states.push_back(st);
        state = &states.back();
        state->ply_from_root = 0;
    }

    [[nodiscard]] bool is_square_attacked_by(const uint8_t index, const bool side) const
    {
        if (!side) {
            if (get_rook_attack(index, occupations[2]) & (boards[R] | boards[Q])) return true;
            if (get_bishop_attack(index, occupations[2]) & (boards[B] | boards[Q])) return true;
            if (knight_attack_tables[index] & boards[N]) return true;
            if (pawn_attack_tables[black][index] & boards[P]) return true;
            if (king_attack_tables[index] & boards[K]) return true;
        }
        else {
            if (get_rook_attack(index, occupations[2]) & (boards[r] | boards[q])) return true;
            if (get_bishop_attack(index, occupations[2]) & (boards[b] | boards[q])) return true;
            if (knight_attack_tables[index] & boards[n]) return true;
            if (pawn_attack_tables[white][index] & boards[p]) return true;
            if (king_attack_tables[index] & boards[k]) return true;
        }
        return false;
    }

    [[nodiscard]] bool is_king_in_check() const
    {
        return is_square_attacked_by(least_significant_one(boards[side_to_move == white ? K : k]), 1 - side_to_move);
    }

    [[nodiscard]] uint64_t get_pinned_board_of(const bool side) const
    {
        uint64_t attacker = 0, pinned_board = 0;
        const uint8_t king_index = least_significant_one(side == white ? boards[K] : boards[k]);

        attacker = (get_rook_attack(king_index, occupations[1 - side]) & (side == white ? (boards[r] | boards[q]) : (boards[R] | boards[Q])))
                     | (get_bishop_attack(king_index, occupations[1 - side]) & (side == white ? (boards[b] | boards[q]) : (boards[B] | boards[Q])));

        while (attacker) {
            const uint8_t sniper = least_significant_one(attacker);

            if (const uint64_t between = lines_between[sniper][king_index] & occupations[side]; std::has_single_bit(between))
                pinned_board |= between;

            attacker &= attacker - 1;
        }

        return pinned_board;
    }

    [[nodiscard]] uint64_t get_checker_of(const bool side) const
    {
        const uint8_t king_index = least_significant_one(side == white ? boards[K] : boards[k]);
        const uint64_t checkers = (pawn_attack_tables[side][king_index] & (side == white ? boards[p] : boards[P]))
            | (knight_attack_tables[king_index] & (side == white ? boards[n] : boards[N]))
            | (get_bishop_attack(king_index, occupations[2]) & (side == white ? (boards[b] | boards[q]) : (boards[B] | boards[Q])))
            | (get_rook_attack(king_index, occupations[2]) & (side == white ? (boards[r] | boards[q]) : (boards[R] | boards[Q])));
        return checkers;
    }

    [[nodiscard]] uint64_t get_check_blocker_of(const bool side) const
    {
        const uint64_t checker = get_checker_of(side);
        return checker | lines_between[least_significant_one(checker)][least_significant_one(side == white ? boards[K] : boards[k])];
    }

    [[nodiscard]] bool is_legal(const Move& move)
    {
        const bool us = side_to_move;
        const bool enemy = 1 - us;
        const uint8_t from = move.src();
        const uint8_t to = move.dest();
        const uint8_t flag = move.flag();
        const uint64_t king_board = boards[us == white ? K : k];
        const uint8_t king = least_significant_one(king_board);

        if (flag == king_castle) {
            for (uint8_t k = from; k <= from + 2; k++) {
                if (is_square_attacked_by(k, enemy)) return false;
            }
        }
        else if (flag == queen_castle) {
            for (uint8_t k = from; k >= from - 2; k--) {
                if (is_square_attacked_by(k, enemy)) return false;
            }
        }
        else if (flag == ep_capture) {
            const uint64_t eq = boards[us == white ? q : Q];
            const uint64_t eb = boards[us == white ? b : B];
            const uint64_t er = boards[us == white ? r : R];
            const uint64_t occ = (occupations[2] ^ (1ull << (to + (us == white ? 8 : -8))) ^ (1ull << from)) | (1ull << to);

            return !((get_bishop_attack(king, occ) & (eq | eb)) | (get_rook_attack(king, occ) & (eq | er)));
        }
        else if (piece_on[from] == k || piece_on[from] == K) {
            occupations[2] ^= king_board;
            if (is_square_attacked_by(to, enemy)) {
                occupations[2] ^= king_board;
                return false;
            }
            occupations[2] ^= king_board;
        }

        return (!(state->pinned & (1ull << from)) || (lines_intersect[from][to] & king_board));
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
    std::cout << "\nSide to move: " << (position.side_to_move == white ? "White\n" : "Black\n");
}