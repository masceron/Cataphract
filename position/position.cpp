#include "position.hpp"
#include "movegen.hpp"

bool Position::is_pseudo_legal(const Move &move)
{
    if (const uint16_t flag = move.flag(); flag >= knight_promotion || flag == ep_capture || flag == queen_castle || flag == king_castle) {
        MoveList list;
        pseudo_legals<all>(*this, list);
        for (int i = 0; i < list.size(); i++) {
            if (list.list[i] == move) return true;
        }
        return false;
    }
    const int8_t from = move.src();
    const int8_t to = move.flag();
    const Pieces moved_piece = piece_on[from];

    if (moved_piece == nil || color_of(moved_piece) != side_to_move) return false;

    const uint64_t to_board = 1ull << to;

    if (occupations[side_to_move] & to_board) return false;

    static constexpr uint64_t rank18 = 0xFF000000000000FFull;
    if (moved_piece == P || moved_piece == p) {
        if (rank18 & boards[moved_piece]) return false;

        if (!(pawn_attack_tables[side_to_move][from] & occupations[!side_to_move] & to_board)
            && !(from + Delta<Up>(side_to_move) == to && piece_on[to] == nil)
            && (from + 2 * Delta<Up>(side_to_move) == to && abs(from - to) == 16 && piece_on[to] != nil && piece_on[to - Delta<Up>(side_to_move)] == nil))
            return false;
    }
    else if (moved_piece == N || moved_piece == n) {
        if (!(knight_attack_tables[from] & to_board)) return false;
    }
    else if (moved_piece == R || moved_piece == r) {
        if (!(get_rook_attack(from, occupations[2]) & to_board)) return false;
    }
    else if (moved_piece == B || moved_piece == b) {
        if (!(get_bishop_attack(from, occupations[2]) & to_board)) return false;
    }
    else if (moved_piece == Q || moved_piece == q) {
        if (!((get_bishop_attack(from, occupations[2]) | get_rook_attack(from, occupations[2])) & to_board)) return false;
    }
    else if (moved_piece == K || moved_piece == k) {
        if (!(king_attack_tables[from] & to_board)) return false;
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

bool Position::is_legal(const Move &move)
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
