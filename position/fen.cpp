#include <string>
#include "position.hpp"
#include "zobrist.hpp"

int algebraic_to_num(const std::string &algebraic)
{
    if (algebraic.length() != 2) return -1;
    int rank = 0;
    int file = 0;
    if (algebraic[1] >= '1' && algebraic[1] <= '8') rank = 8 - static_cast<int>(algebraic[1] - '0');
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

std::string num_to_algebraic(const uint8_t sq)
{
    constexpr char files[] = "abcdefgh";

    return files[sq % 8] + std::to_string(8 - sq/8);
}

int fen_parse(std::string fen)
{
    fen.erase(fen.begin(), std::find_if(fen.begin(), fen.end(), [](const unsigned char ch) {
        return !std::isspace(ch);
    }));
    fen.erase(std::find_if(fen.rbegin(), fen.rend(), [](const unsigned char ch) {
        return !std::isspace(ch);
    }).base(), fen.end());

    position.new_game();
    if (fen == "startpos") fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    uint64_t cr_pts = 0;
    uint64_t cr_chars = 0;
    while (fen[cr_chars] != ' ') {
        switch (fen[cr_chars]) {
            case 'p':
                set_bit(position.boards[p], cr_pts);
                position.piece_on[cr_pts] = p;
                position.state->pawn_key ^= zobrist_keys.pawn_keys[z_p][cr_pts - 8];
                cr_pts++;
                cr_chars++;
            break;
            case 'r':
                set_bit(position.boards[r], cr_pts);
                position.piece_on[cr_pts] = r;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_r][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'b':
                set_bit(position.boards[b], cr_pts);
                position.piece_on[cr_pts] = b;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_b][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'n':
                set_bit(position.boards[n], cr_pts);
                position.piece_on[cr_pts] = n;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_n][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'k':
                set_bit(position.boards[k], cr_pts);
                position.piece_on[cr_pts] = k;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_k][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'q':
                set_bit(position.boards[q], cr_pts);
                position.piece_on[cr_pts] = q;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_q][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'P':
                set_bit(position.boards[P], cr_pts);
                position.piece_on[cr_pts] = P;
                position.state->pawn_key ^= zobrist_keys.pawn_keys[z_P][cr_pts - 8];
                cr_pts++;
                cr_chars++;
            break;
            case 'R':
                set_bit(position.boards[R], cr_pts);
                position.piece_on[cr_pts] = R;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_R][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'B':
                set_bit(position.boards[B], cr_pts);
                position.piece_on[cr_pts] = B;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_B][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'N':
                set_bit(position.boards[N], cr_pts);
                position.piece_on[cr_pts] = N;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_N][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'K':
                set_bit(position.boards[K], cr_pts);
                position.piece_on[cr_pts] = K;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_K][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'Q':
                set_bit(position.boards[Q], cr_pts);
                position.piece_on[cr_pts] = Q;
                position.state->non_pawn_key ^= zobrist_keys.non_pawn_keys[z_Q][cr_pts];
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

    if (position.side_to_move == black)
        position.state->side_key ^= zobrist_keys.side_key;

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

    position.state->castling_key ^= zobrist_keys.castling_keys[position.state->castling_rights];

    if (fen[cr_chars] != ' ') {
        position.new_game();
        return -1;
    }
    cr_chars++;

    if (fen[cr_chars] != '-') {
        const std::string en_passant{fen[cr_chars], fen[cr_chars + 1]};
        if (position.state->en_passant_square = algebraic_to_num(en_passant); position.state->en_passant_square == -1) {
            position.new_game();
            return -1;
        }
        cr_chars += 2;
    }
    else
        cr_chars +=1;


    if (position.state->en_passant_square != -1)
        position.state->en_passant_key ^= zobrist_keys.en_passant_key[position.state->en_passant_square % 8];


    if (cr_chars >= fen.length()) {
        position.state->rule_50 = 0;
        position.state->ply = 0;
    }
    else {
        cr_chars++;
        const auto next = fen.find(' ', cr_chars);
        if (next == std::string::npos) {
            position.new_game();
            return -1;
        }
        try {
            position.state->rule_50 = std::stoi(fen.substr(cr_chars, next - cr_chars));
            position.state->ply = 0;
        } catch (...) {
            position.new_game();
            return -1;
        }
    }

    position.occupations[white] = position.boards[P] | position.boards[K] | position.boards[N] | position.boards[Q] | position.boards[B] | position.boards[R];
    position.occupations[black] = position.boards[p] | position.boards[k] | position.boards[n] | position.boards[q] | position.boards[b] | position.boards[r];
    position.occupations[2] = position.occupations[white] | position.occupations[black];
    position.pinned[white] = get_pinned_board_of(white);
    position.pinned[black] = get_pinned_board_of(black);
    if (is_king_in_check_by(1 - position.side_to_move))
        position.checker = get_checker_of(position.side_to_move);

    position.state->key = position.state->pawn_key ^ position.state->non_pawn_key ^ position.state->castling_key ^ position.state->en_passant_key ^ position.state->side_key;

    return 0;
}
