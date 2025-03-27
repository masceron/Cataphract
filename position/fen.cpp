#include <string>
#include "position.h"

int algebraic_to_num(const std::string &algebraic)
{
    if (algebraic.length() != 2) return -1;
    int rank = 0;
    int file = 0;
    if (algebraic[1] >= '1' && algebraic[1] <= '8') rank = 8 - static_cast<int>(algebraic[1]);
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

int fen_parse(std::string fen)
{
    position.new_game();
    if (fen == "startpos") fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int cr_pts = 0;
    uint64_t cr_chars = 0;
    while (fen[cr_chars] != ' ') {
        switch (fen[cr_chars]) {
            case 'p':
                set_bit(position.boards[p], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'r':
                set_bit(position.boards[r], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'b':
                set_bit(position.boards[b], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'n':
                set_bit(position.boards[n], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'k':
                set_bit(position.boards[k], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'q':
                set_bit(position.boards[q], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'P':
                set_bit(position.boards[P], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'R':
                set_bit(position.boards[R], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'B':
                set_bit(position.boards[B], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'N':
                set_bit(position.boards[N], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'K':
                set_bit(position.boards[K], cr_pts);
                cr_pts++;
                cr_chars++;
            break;
            case 'Q':
                set_bit(position.boards[Q], cr_pts);
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
                position.new_game();
                return -1;
        }
    }
    if (cr_pts != 64) {
        position.new_game();
        return -1;
    }
    cr_chars++;
    switch (fen[cr_chars]) {
        case 'w':
            position.side_to_move = white;
        break;
        case 'b':
            position.side_to_move = black;
        break;
        default:
            position.new_game();
            return -1;
    }
    cr_chars++;
    if (fen[cr_chars] != ' ') {
        position.new_game();
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
                position.state->castling_rights |= white_king;
                cr_chars++;
                counter++;
            break;
            case 'Q':
                position.state->castling_rights |= white_queen;
                cr_chars++;
                counter++;
            break;
            case 'k':
                position.state->castling_rights |= black_king;
                cr_chars++;
                counter++;
            break;
            case 'q':
                position.state->castling_rights |= black_queen;
                cr_chars++;
                counter++;
            break;
            case ' ':
                counter = 4;
            break;
            default:
                position.new_game();
                return -1;
        }
    if (fen[cr_chars] != ' ') {
        position.new_game();
        return -1;
    }
    cr_chars++;
    if (const std::string en_passant{fen[cr_chars], fen[cr_chars + 1]}; en_passant != "- ") {
        if (position.state->en_passant_square = algebraic_to_num(en_passant); position.state->en_passant_square == -1) {
            position.new_game();
            return -1;
        }
        cr_chars += 2;
    }
    else
        cr_chars +=1;
    if (cr_chars >= fen.length()) {
        position.half_moves = 0;
        position.full_moves = 1;
    }
    else if (fen[cr_chars] != ' ') {
        position.new_game();
        return -1;
    }
    cr_chars++;
    const auto next = fen.find(' ', cr_chars);
    if (next == std::string::npos) {
        position.new_game();
        return -1;
    }
    try {
        position.half_moves = std::stoi(fen.substr(cr_chars, next - cr_chars));
    } catch (...) {
        position.new_game();
        return -1;
    }
    try {
        position.full_moves = std::stoi(fen.substr(next + 1, std::string::npos));
    } catch (...) {
        position.new_game();
        return -1;
    }
    position.occupations[white] = position.boards[P] | position.boards[K] | position.boards[N] | position.boards[Q] | position.boards[B] | position.boards[R];
    position.occupations[black] = position.boards[p] | position.boards[k] | position.boards[n] | position.boards[q] | position.boards[b] | position.boards[r];
    position.occupations[both] = position.occupations[white] | position.occupations[black];
    position.pinned[white] = get_pinned_board_of(white);
    position.pinned[black] = get_pinned_board_of(black);
    position.checker[position.side_to_move] = get_checker_of(position.side_to_move);
    return 0;
}
