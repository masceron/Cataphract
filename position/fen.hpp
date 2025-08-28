#pragma once
#include <cstdint>
#include <string>

#include "zobrist.hpp"
#include "position.hpp"
#include "move.hpp"

inline void setup_state(State& st)
{
    st.captured_piece = nil;
    st.castling_key = 0;
    st.castling_rights = 0;
    st.en_passant_key = 0;
    st.piece_key = 0;
    st.en_passant_square = -1;
    st.previous = nullptr;
    st.side_key = 0;
    st.repetition = 1;
    st.checker = 0;
    st.check_blocker = 0;
}

inline int fen_parse(std::string fen)
{
    State st;
    setup_state(st);
    Position temp;
    temp.state = &st;

    fen.erase(fen.begin(), std::ranges::find_if(fen, [](const unsigned char ch) {
        return !std::isspace(ch);
    }));
    fen.erase(std::ranges::find_if(fen.rbegin(), fen.rend(), [](const unsigned char ch) {
        return !std::isspace(ch);
    }).base(), fen.end());

    if (fen == "startpos") fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    uint64_t cr_pts = 0;
    uint64_t cr_chars = 0;
    while (fen[cr_chars] != ' ') {
        switch (fen[cr_chars]) {
            case 'p':
                if (cr_pts < 8 || cr_pts > 55) return -1;
                set_bit(temp.boards[p], cr_pts);
                temp.piece_on[cr_pts] = p;
                st.piece_key ^= Zobrist::piece_keys[p][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'r':
                set_bit(temp.boards[r], cr_pts);
                temp.piece_on[cr_pts] = r;
                st.piece_key ^= Zobrist::piece_keys[r][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'b':
                set_bit(temp.boards[b], cr_pts);
                temp.piece_on[cr_pts] = b;
                st.piece_key ^= Zobrist::piece_keys[b][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'n':
                set_bit(temp.boards[n], cr_pts);
                temp.piece_on[cr_pts] = n;
                st.piece_key ^= Zobrist::piece_keys[n][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'k':
                set_bit(temp.boards[k], cr_pts);
                temp.piece_on[cr_pts] = k;
                st.piece_key ^= Zobrist::piece_keys[k][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'q':
                set_bit(temp.boards[q], cr_pts);
                temp.piece_on[cr_pts] = q;
                st.piece_key ^= Zobrist::piece_keys[q][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'P':
                if (cr_pts < 8 || cr_pts > 55) return -1;
                set_bit(temp.boards[P], cr_pts);
                temp.piece_on[cr_pts] = P;
                st.piece_key ^= Zobrist::piece_keys[P][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'R':
                set_bit(temp.boards[R], cr_pts);
                temp.piece_on[cr_pts] = R;
                st.piece_key ^= Zobrist::piece_keys[R][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'B':
                set_bit(temp.boards[B], cr_pts);
                temp.piece_on[cr_pts] = B;
                st.piece_key ^= Zobrist::piece_keys[B][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'N':
                set_bit(temp.boards[N], cr_pts);
                temp.piece_on[cr_pts] = N;
                st.piece_key ^= Zobrist::piece_keys[N][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'K':
                set_bit(temp.boards[K], cr_pts);
                temp.piece_on[cr_pts] = K;
                st.piece_key ^= Zobrist::piece_keys[K][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'Q':
                set_bit(temp.boards[Q], cr_pts);
                temp.piece_on[cr_pts] = Q;
                st.piece_key ^= Zobrist::piece_keys[Q][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case '1':
                cr_pts += 1;
                cr_chars++;
            break;
            case '2':
                cr_pts += 2;
                cr_chars++;
            break;
            case '3':
                cr_pts += 3;
                cr_chars++;
            break;
            case '4':
                cr_pts += 4;
                cr_chars++;
            break;
            case '5':
                cr_pts += 5;
                cr_chars++;
            break;
            case '6':
                cr_pts += 6;
                cr_chars++;
            break;
            case '7':
                cr_pts += 7;
                cr_chars++;
            break;
            case '8':
                cr_pts += 8;
                cr_chars++;
            break;
            case '/':
                cr_chars++;
            break;
            default:
                return -1;
        }
    }
    if (cr_pts != 64) {
        return -1;
    }
    cr_chars++;
    switch (fen[cr_chars]) {
        case 'w':
            temp.side_to_move = white;
        break;
        case 'b':
            temp.side_to_move = black;
        break;
        default:
            return -1;
    }
    cr_chars++;

    if (temp.side_to_move == black)
        st.side_key ^= Zobrist::side_key;

    if (fen[cr_chars] != ' ') {
        return -1;
    }
    cr_chars++;
    int counter = 0;
    if (fen[cr_chars] == '-') {
        counter = 4;
        cr_chars++;
    }
    while (counter < 4)
        switch (fen[cr_chars]) {
            case 'K':
                st.castling_rights |= white_king;
                cr_chars++;
                counter++;
            break;
            case 'Q':
                st.castling_rights |= white_queen;
                cr_chars++;
                counter++;
            break;
            case 'k':
                st.castling_rights |= black_king;
                cr_chars++;
                counter++;
            break;
            case 'q':
                st.castling_rights |= black_queen;
                cr_chars++;
                counter++;
            break;
            case ' ':
                counter = 4;
            break;
            default:
                return -1;
        }

    st.castling_key ^= Zobrist::castling_keys[st.castling_rights];

    if (fen[cr_chars] != ' ') {
        return -1;
    }
    cr_chars++;

    if (fen[cr_chars] != '-') {
        const std::string en_passant{fen[cr_chars], fen[cr_chars + 1]};
        if (st.en_passant_square = algebraic_to_num(en_passant); st.en_passant_square == -1) {
            return -1;
        }
        cr_chars += 2;
    }
    else
        cr_chars +=1;


    if (st.en_passant_square != -1)
        st.en_passant_key ^= Zobrist::en_passant_key[st.en_passant_square % 8];


    if (cr_chars >= fen.length()) {
        st.rule_50 = 0;
        st.ply = 0;
    }
    else {
        cr_chars++;
        const auto next = fen.find(' ', cr_chars);
        if (next == std::string::npos) {
            return -1;
        }
        st.rule_50 = std::atoi(fen.substr(cr_chars, next - cr_chars).c_str());
        st.ply = 0;
    }

    temp.occupations[white] = temp.boards[P] | temp.boards[K] | temp.boards[N] | temp.boards[Q] | temp.boards[B] | temp.boards[R];
    temp.occupations[black] = temp.boards[p] | temp.boards[k] | temp.boards[n] | temp.boards[q] | temp.boards[b] | temp.boards[r];
    temp.occupations[2] = temp.occupations[white] | temp.occupations[black];

    if (!temp.side_to_move) {
        st.checker = temp.get_checker_of<white>();
        st.pinned = temp.get_pinned_board_of<white>();
    }
    else {
        st.checker = temp.get_checker_of<black>();
        st.pinned = temp.get_pinned_board_of<black>();
    }

    if (st.checker) {
        st.check_blocker = temp.get_check_blocker_of(temp.side_to_move);
    }

    st.key = st.piece_key ^ st.castling_key ^ st.en_passant_key ^ st.side_key;

    position.new_game();
    states.clear();
    states.push_back(st);
    temp.state = &states.back();

    position = temp;

    return 0;
}
