#include <sstream>

#include "position.hpp"

#include "cuckoo.hpp"
#include "movegen.hpp"
#include "../search/history.hpp"

void Position::new_game()
{
    std::fill_n(piece_on, 64, nil);
    std::fill_n(boards, 12, 0);
    std::fill_n(occupations, 3, 0);
}

void Position::move_piece(const uint8_t from, const uint8_t to)
{
    const Pieces piece = piece_on[from];
    const uint64_t ft = (1ull << from) | (1ull << to);
    boards[piece] ^= ft;
    occupations[color_of(piece)] ^= ft;
    occupations[2] ^= ft;
    piece_on[to] = piece;
    piece_on[from] = nil;
}

void Position::put_piece(const Pieces piece, const uint8_t sq)
{
    piece_on[sq] = piece;
    const uint64_t board = 1ull << sq;
    occupations[color_of(piece)] |= board;
    occupations[2] |= board;
    boards[piece] |= board;
}

void Position::remove_piece(const uint8_t sq)
{
    const Pieces piece = piece_on[sq];
    const uint64_t board = 1ull << sq;
    piece_on[sq] = nil;
    occupations[color_of(piece)] ^= board;
    occupations[2] ^= board;
    boards[piece] ^= board;
}

void Position::make_move(const Move move, State& st)
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

    Continuation::search_stack.push_back(((moving_piece >= p ? moving_piece - 6 : moving_piece) << 6) + to);

    if (flag == king_castle) {
        move_piece(to + 1, to - 1);
        const Pieces rook = side_to_move == white ? R : r;

        st.piece_key ^= Zobrist::piece_keys[rook][to + 1] ^ Zobrist::piece_keys[rook][to - 1];
    }
    else if (flag == queen_castle) {
        const Pieces rook = side_to_move == white ? R : r;

        move_piece(to - 2, to + 1);

        st.piece_key ^= Zobrist::piece_keys[rook][to - 2] ^ Zobrist::piece_keys[rook][to + 1];
    }

    else if (captured_piece != nil) {
        if ((captured_piece == P || captured_piece == p) && flag == ep_capture) {
                const uint8_t captured_square = to + (side_to_move == white ? 8 : -8);
                remove_piece(captured_square);

                st.piece_key ^= Zobrist::piece_keys[captured_piece][captured_square];
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

            st.piece_key ^= Zobrist::piece_keys[captured_piece][to];
        }
        st.rule_50 = 0;
    }

    if (st.en_passant_square != -1) {
        st.en_passant_key = 0;
        st.en_passant_square = -1;
    }

    move_piece(from, to);

    if (moving_piece == P || moving_piece == p) {
        if (flag == double_push &&
            (pawn_attack_tables[side_to_move][to + (side_to_move == white ? 8 : -8)] & boards[side_to_move == white ? p : P])) {
            st.en_passant_square = to + (side_to_move == white ? 8 : -8);

            st.en_passant_key ^= Zobrist::en_passant_key[st.en_passant_square % 8];
        }
        else if (flag >= knight_promotion) {
            const Pieces promoted_to = move.promoted_to<true>(side_to_move);
            remove_piece(to);
            put_piece(promoted_to, to);

            st.piece_key ^= Zobrist::piece_keys[promoted_to][to] ^ Zobrist::piece_keys[moving_piece][to];
        }

        st.rule_50 = 0;
    }
    else {
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

    st.piece_key ^= Zobrist::piece_keys[moving_piece][from] ^ Zobrist::piece_keys[moving_piece][to];

    st.side_key ^= Zobrist::side_key;

    st.key = st.piece_key ^ st.castling_key ^ st.en_passant_key ^ st.side_key;

    if (!side_to_move) {
        st.checker = get_checker_of<black>();
        st.pinned = get_pinned_board_of<black>();
    }
    else {
        st.checker = get_checker_of<white>();
        st.pinned = get_pinned_board_of<white>();
    }

    if (st.checker) {
        st.check_blocker = get_check_blocker_of(!side_to_move);
    }

    side_to_move = !side_to_move;

    st.repetition = 1;
    if (const uint16_t cutoff = std::min(static_cast<uint16_t>(st.rule_50), st.ply); cutoff >= 4) {
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

void Position::do_move(const Move move)
{
    State st;
    make_move(move, st);
    states.push_back(st);
    state = &states.back();
    state->ply_from_root = 0;
}

void Position::unmake_move(const Move &move)
{
    Continuation::search_stack.pop_back();
    side_to_move = !side_to_move;
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
            captured_square -= Delta<Up>(side_to_move);
        }

        put_piece(state->captured_piece, captured_square);
    }

    state = state->previous;
}

bool Position::is_pseudo_legal(const Move move)
{
    const uint16_t flag = move.flag();
    if (flag >= knight_promotion || flag == ep_capture || flag == queen_castle || flag == king_castle) {
        MoveList list;
        pseudo_legals<all>(*this, list);
        for (int i = 0; i < list.size(); i++) {
            if (list.list[i] == move) return true;
        }
        return false;
    }
    const int8_t from = move.src();
    const int8_t to = move.dest();
    const Pieces moved_piece = piece_on[from];

    if (moved_piece == nil || color_of(moved_piece) != side_to_move) return false;
    if ((flag == capture && piece_on[to] == nil) || (flag != capture && piece_on[to] != nil)) return false;

    const uint64_t to_board = 1ull << to;

    if (occupations[side_to_move] & to_board) return false;

    static constexpr uint64_t rank18 = 0xFF000000000000FFull;
    if (moved_piece == P || moved_piece == p) {
        if (rank18 & to_board) return false;

        if (!(pawn_attack_tables[side_to_move][from] & occupations[!side_to_move] & to_board)
            && !(from + Delta<Up>(side_to_move) == to && piece_on[to] == nil)
            && !(from + 2 * Delta<Up>(side_to_move) == to && piece_on[to] == nil && piece_on[to - Delta<Up>(side_to_move)] == nil))
            return false;
    }
    else if (moved_piece == N || moved_piece == n) {
        if (!(knight_attack_tables[from] & to_board & ~occupations[side_to_move])) return false;
    }
    else if (moved_piece == R || moved_piece == r) {
        if (!(get_rook_attack(from, occupations[2]) & to_board & ~occupations[side_to_move])) return false;
    }
    else if (moved_piece == B || moved_piece == b) {
        if (!(get_bishop_attack(from, occupations[2]) & to_board & ~occupations[side_to_move])) return false;
    }
    else if (moved_piece == Q || moved_piece == q) {
        if (!((get_bishop_attack(from, occupations[2]) | get_rook_attack(from, occupations[2])) & to_board & ~occupations[side_to_move])) return false;
    }
    else if (moved_piece == K || moved_piece == k) {
        if (!(king_attack_tables[from] & to_board & ~occupations[side_to_move])) return false;
    }

    if (state->checker) {
        if (moved_piece != K && moved_piece != k) {
            if (std::popcount(state->checker) != 1) {
                return false;
            }

            if (!(state->check_blocker & to_board)) return false;
        }
        else {
            occupations[2] ^= boards[moved_piece];
            if (is_square_attacked_by(to, !side_to_move)) {
                occupations[2] ^= boards[moved_piece];
                return false;
            }
            occupations[2] ^= boards[moved_piece];
        }
    }

    return true;
}

bool Position::is_legal(const Move move)
{
    const bool us = side_to_move;
    const bool enemy = !us;
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

uint64_t Position::construct_zobrist_key() const
{
    uint64_t key = 0;
    for (int8_t piece = 0; piece < 12; piece++) {
        uint64_t board = boards[piece];
        while (board) {
            key ^= Zobrist::piece_keys[piece][least_significant_one(board)];
            board &= board - 1;
        }
    }
    if (side_to_move) key ^= Zobrist::side_key;
    if (state->en_passant_square != -1) key ^= Zobrist::en_passant_key[state->en_passant_square % 8];
    key ^= Zobrist::castling_keys[state->castling_rights];

    return key;
}

void Position::make_null_move(State& st)
{
    memcpy(&st, state, sizeof(State));

    st.previous = state;
    st.repetition = 1;
    st.ply_from_root++;
    st.ply = 0;
    st.key ^= Zobrist::side_key;
    st.side_key ^= Zobrist::side_key;

    if (st.en_passant_square != -1) {
        st.en_passant_square = -1;
        st.key ^= st.en_passant_key;
        st.en_passant_key = 0;
    }

    side_to_move = !side_to_move;

    if (side_to_move == white) st.pinned = get_pinned_board_of<white>();
    else st.pinned = get_pinned_board_of<black>();
    state = &st;
}

void Position::unmake_null_move()
{
    state = state->previous;
    side_to_move = !side_to_move;
}

bool Position::is_square_attacked_by(const uint8_t index, const bool side) const
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


uint64_t Position::get_check_blocker_of(const bool side) const
{
    return state->checker | lines_between[least_significant_one(state->checker)][least_significant_one(side == white ? boards[K] : boards[k])];
}

bool Position::upcoming_repetition(const uint8_t ply) const
{
    const uint8_t end = std::min(static_cast<uint16_t>(state->rule_50), state->ply);

    if (end < 3) return false;

    const uint64_t original_key = state->key;
    const State* prev = state->previous;
    uint64_t other = original_key ^ prev->key ^ Zobrist::side_key;

    for (int i = 3; i <= end; i+=2) {
        prev = prev->previous;
        other ^= prev->key ^ prev->previous->key ^ Zobrist::side_key;
        prev = prev->previous;

        if (other != 0) continue;

        const uint64_t diff = original_key ^ prev->key;
        uint64_t slot = Cuckoo::hash1(diff);

        if (diff != Cuckoo::cuckoo_key[slot]) slot = Cuckoo::hash2(diff);
        if (diff != Cuckoo::cuckoo_key[slot]) continue;

        if (const Move move = Cuckoo::cuckoo_move[slot]; !(occupations[2] & lines_between[move.src()][move.dest()])) {
            if (ply > i) return true;

            if (prev->repetition > 1) return true;
        }
    }

    return false;
}

void Position::print_board() const
{
    std::string lines[8];
    for (int rank = 0; rank < 8; rank++) {
        lines[rank] += std::to_string(8-rank) + "  ";
        for (int file = 0; file < 8; file++) {
            lines[rank] += ". ";
        }
        lines[rank] += "\n";
    }
    for (int _pos = 0; _pos < 64; _pos++) {
        lines[_pos / 8][3 + (_pos % 8) * 2] = piece_icons[piece_on[_pos]];
    }

    std::cout << "   a b c d e f g h\n";
    for (const auto& i: lines) std::cout << i;
    std::cout << "FEN: " << to_fen();
}

std::string Position::to_fen() const
{
    std::stringstream fen;
    for (int row = 0; row < 8; row++) {
        int empty = 0;
        for (int col = 0; col < 8; col++) {
            if (piece_on[row * 8 + col] != nil) {
                if (empty) {
                    fen << empty;
                    empty = 0;
                }
                fen << piece_icons[piece_on[row * 8 + col]];
            }
            else empty++;
        }
        if (empty) fen << empty;
        fen << "/";
    }
    fen.seekp(-1, std::stringstream::cur);
    fen << " " << (side_to_move ? "b " : "w ");

    if (!state->castling_rights) fen << "-";
    else {
        static constexpr char rights[] = {'K', 'Q', 'k', 'q'};
        for (int i = 0; i < 4; i++) {
            if (state->castling_rights >> i & 1) fen << rights[i];
        }
    }
    fen << " ";

    if (state->en_passant_square == -1) fen << "-";
    else fen << num_to_algebraic(state->en_passant_square);

    fen << " " << static_cast<int>(state->rule_50) << " " << std::max((state->ply + 1) / 2, 1);
    fen << std::endl;
    return fen.str();
}
