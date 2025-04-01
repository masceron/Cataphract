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

void setup_state(State& st)
{
    st.captured_piece = nil;
    st.castling_key = 0;
    st.castling_rights = 0;
    st.en_passant_key = 0;
    st.non_pawn_key = 0;
    st.en_passant_square = -1;
    st.pawn_key = 0;
    st.previous = nullptr;
    st.side_key = 0;
    st.repetition = 1;
}

int fen_parse(std::string fen)
{
    State st;
    setup_state(st);
    Position temp;
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
                set_bit(temp.boards[p], cr_pts);
                temp.piece_on[cr_pts] = p;
                st.pawn_key ^= zobrist_keys.pawn_keys[z_p][cr_pts - 8];
                cr_pts++;
                cr_chars++;
            break;
            case 'r':
                set_bit(temp.boards[r], cr_pts);
                temp.piece_on[cr_pts] = r;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_r][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'b':
                set_bit(temp.boards[b], cr_pts);
                temp.piece_on[cr_pts] = b;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_b][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'n':
                set_bit(temp.boards[n], cr_pts);
                temp.piece_on[cr_pts] = n;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_n][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'k':
                set_bit(temp.boards[k], cr_pts);
                temp.piece_on[cr_pts] = k;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_k][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'q':
                set_bit(temp.boards[q], cr_pts);
                temp.piece_on[cr_pts] = q;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_q][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'P':
                set_bit(temp.boards[P], cr_pts);
                temp.piece_on[cr_pts] = P;
                st.pawn_key ^= zobrist_keys.pawn_keys[z_P][cr_pts - 8];
                cr_pts++;
                cr_chars++;
            break;
            case 'R':
                set_bit(temp.boards[R], cr_pts);
                temp.piece_on[cr_pts] = R;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_R][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'B':
                set_bit(temp.boards[B], cr_pts);
                temp.piece_on[cr_pts] = B;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_B][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'N':
                set_bit(temp.boards[N], cr_pts);
                temp.piece_on[cr_pts] = N;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_N][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'K':
                set_bit(temp.boards[K], cr_pts);
                temp.piece_on[cr_pts] = K;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_K][cr_pts];
                cr_pts++;
                cr_chars++;
            break;
            case 'Q':
                set_bit(temp.boards[Q], cr_pts);
                temp.piece_on[cr_pts] = Q;
                st.non_pawn_key ^= zobrist_keys.non_pawn_keys[z_Q][cr_pts];
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
        st.side_key ^= zobrist_keys.side_key;

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

    st.castling_key ^= zobrist_keys.castling_keys[st.castling_rights];

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
        st.en_passant_key ^= zobrist_keys.en_passant_key[st.en_passant_square % 8];


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
        try {
            st.rule_50 = std::stoi(fen.substr(cr_chars, next - cr_chars));
            st.ply = 0;
        } catch (...) {
            return -1;
        }
    }

    temp.occupations[white] = temp.boards[P] | temp.boards[K] | temp.boards[N] | temp.boards[Q] | temp.boards[B] | temp.boards[R];
    temp.occupations[black] = temp.boards[p] | temp.boards[k] | temp.boards[n] | temp.boards[q] | temp.boards[b] | temp.boards[r];
    temp.occupations[2] = temp.occupations[white] | temp.occupations[black];
    st.pinned = temp.get_pinned_board_of(temp.side_to_move);
    if (temp.is_king_in_check_by(1 - temp.side_to_move)) {
        st.checker = temp.get_checker_of(temp.side_to_move);
        st.check_blocker = temp.get_check_blocker_of(temp.side_to_move);
    }

    st.key = st.pawn_key ^ st.non_pawn_key ^ st.castling_key ^ st.en_passant_key ^ st.side_key;

    position.new_game();
    states.clear();
    states.push_back(st);
    temp.state = &states.back();

    position = temp;

    return 0;
}
