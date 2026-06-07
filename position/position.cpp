#include <sstream>
#include <print>
#include <deque>
#include <algorithm>
#include <cstring>
#include <utility>

#include "position.hpp"
#include "cuckoo.hpp"
#include "zobrist.hpp"
#include "../board/attacks.hpp"
#include "../search/thread.hpp"
#include "../board/lines.hpp"
#include "../board/slider.hpp"

bool color_of(const Piece piece)
{
    return piece >= 6;
}

Position::Position()
{
    new_game();
}

void Position::new_game()
{
    side_to_move = white;
    std::ranges::fill(piece_on, nil);
    std::ranges::fill(boards, 0);
    std::ranges::fill(occupations, 0);
}

void Position::move_piece(const int from, const int to)
{
    const Piece piece = piece_on[from];
    const uint64_t ft = 1ull << from | 1ull << to;
    boards[piece] ^= ft;
    occupations[color_of(piece)] ^= ft;
    occupations[2] ^= ft;
    piece_on[to] = piece;
    piece_on[from] = nil;
}

void Position::put_piece(const Piece piece, const int sq)
{
    piece_on[sq] = piece;
    const uint64_t board = 1ull << sq;
    occupations[color_of(piece)] |= board;
    occupations[2] |= board;
    boards[piece] |= board;
}

void Position::remove_piece(const int sq)
{
    const Piece piece = piece_on[sq];
    const uint64_t board = 1ull << sq;
    piece_on[sq] = nil;
    occupations[color_of(piece)] ^= board;
    occupations[2] ^= board;
    boards[piece] ^= board;
}

void Position::make_move(const Move move, State& st)
{
    std::memcpy(&st, state, offsetof(State, key));
    st.previous = state;
    st.rule_50++;
    st.ply++;
    state = &st;

    const uint8_t from = move.from();
    const uint8_t to = move.to();
    const auto flag = move.flag();
    const Piece moving_piece = piece_on[from];
    const Piece captured_piece = flag == MoveFlag::ep_capture ? (side_to_move == white ? p : P) : piece_on[to];

    uint64_t key = st.previous->key ^ Zobrist::side_key ^ Zobrist::castling_keys[st.castling_rights];

    if (captured_piece != nil)
    {
        auto captured_square = to;
        if (type_of(captured_piece) == Pawn)
        {
            if (flag == MoveFlag::ep_capture)
            {
                captured_square = to + (side_to_move == white ? 8 : -8);
            }

            remove_piece(captured_square);
            st.pawn_key ^= Zobrist::piece_keys[captured_piece][captured_square];
        }
        else
        {
            remove_piece(captured_square);
            st.non_pawn_keys[!side_to_move] ^= Zobrist::piece_keys[captured_piece][captured_square];

            if (type_of(captured_piece) >= Rook)
            {
                st.major_key ^= Zobrist::piece_keys[captured_piece][captured_square];
            }
        }

        if (st.castling_rights)
        {
            if (side_to_move == white)
            {
                if (captured_square == 0)
                {
                    st.castling_rights &= ~black_queen;
                }
                else if (captured_square == 7)
                {
                    st.castling_rights &= ~black_king;
                }
            }
            else
            {
                if (captured_square == 56)
                {
                    st.castling_rights &= ~white_queen;
                }
                else if (captured_square == 63)
                {
                    st.castling_rights &= ~white_king;
                }
            }
        }

        key ^= Zobrist::piece_keys[captured_piece][captured_square];
        st.rule_50 = 0;
    }
    else
    {
        const Piece rook_piece = (side_to_move == white) ? R : r;
        switch (flag)
        {
        case MoveFlag::king_castle:
            move_piece(to + 1, to - 1);
            st.non_pawn_keys[side_to_move] ^= Zobrist::piece_keys[rook_piece][to + 1] ^ Zobrist::piece_keys[rook_piece][to - 1];
            st.major_key ^= Zobrist::piece_keys[rook_piece][to + 1] ^ Zobrist::piece_keys[rook_piece][to - 1];
            key ^= Zobrist::piece_keys[rook_piece][to + 1] ^ Zobrist::piece_keys[rook_piece][to - 1];
            break;
        case MoveFlag::queen_castle:
            move_piece(to - 2, to + 1);
            st.non_pawn_keys[side_to_move] ^= Zobrist::piece_keys[rook_piece][to - 2] ^ Zobrist::piece_keys[rook_piece][to + 1];
            st.major_key ^= Zobrist::piece_keys[rook_piece][to - 2] ^ Zobrist::piece_keys[rook_piece][to + 1];
            key ^= Zobrist::piece_keys[rook_piece][to - 2] ^ Zobrist::piece_keys[rook_piece][to + 1];
            break;
        default: break;
        }
    }

    if (st.en_passant_square != -1) [[unlikely]]
    {
        key ^= Zobrist::en_passant_key[st.en_passant_square % 8];
        st.en_passant_square = -1;
    }

    move_piece(from, to);
    key ^= Zobrist::piece_keys[moving_piece][from] ^ Zobrist::piece_keys[moving_piece][to];

    if (type_of(moving_piece) == Pawn)
    {
        st.pawn_key ^= Zobrist::piece_keys[moving_piece][from] ^ Zobrist::piece_keys[moving_piece][to];

        if (flag >= MoveFlag::knight_promotion)
        {
            const Piece promoted_to = move.promoted_to(side_to_move);
            remove_piece(to);
            put_piece(promoted_to, to);

            st.pawn_key ^= Zobrist::piece_keys[moving_piece][to];
            st.non_pawn_keys[side_to_move] ^= Zobrist::piece_keys[promoted_to][to];
            key ^= Zobrist::piece_keys[moving_piece][to] ^ Zobrist::piece_keys[promoted_to][to];

            if (type_of(promoted_to) >= Rook) st.major_key ^= Zobrist::piece_keys[promoted_to][to];
        }
        else if (flag == MoveFlag::double_push &&
            pawn_attack_tables[side_to_move][to + (side_to_move == white ? 8 : -8)] & boards[
                side_to_move == white ? p : P])
        {
            st.en_passant_square = static_cast<int8_t>(to + (side_to_move == white ? 8 : -8));
            key ^= Zobrist::en_passant_key[st.en_passant_square % 8];
        }

        st.rule_50 = 0;
    }
    else
    {
        st.non_pawn_keys[side_to_move] ^= Zobrist::piece_keys[moving_piece][from] ^ Zobrist::piece_keys[moving_piece][
            to];
        if (type_of(moving_piece) >= Rook)
        {
            st.major_key ^= Zobrist::piece_keys[moving_piece][from] ^ Zobrist::piece_keys[moving_piece][to];
        }
        if (st.castling_rights)
        {
            if (moving_piece == K && from == 60)
            {
                st.castling_rights &= 0b1100;
            }
            else if (moving_piece == k && from == 4)
            {
                st.castling_rights &= 0b0011;
            }
            else if (moving_piece == R)
            {
                if (from == 56)
                {
                    st.castling_rights &= ~white_queen;
                }
                else if (from == 63)
                {
                    st.castling_rights &= ~white_king;
                }
            }
            else if (moving_piece == r)
            {
                if (from == 7)
                {
                    st.castling_rights &= ~black_king;
                }
                else if (from == 0)
                {
                    st.castling_rights &= ~black_queen;
                }
            }
        }
    }

    key ^= Zobrist::castling_keys[st.castling_rights];

    st.captured_piece = captured_piece;

    side_to_move = !side_to_move;

    get_checks();
    st.attacks = UINT64_MAX;
    st.pinned = UINT64_MAX;
    st.key = key;

    st.repetition = 1;
    if (const uint16_t cutoff = std::min(static_cast<uint16_t>(st.rule_50), st.ply); cutoff >= 4)
    {
        const State* tst = st.previous->previous;
        for (int cr = 4; cr <= cutoff; cr += 2)
        {
            tst = tst->previous->previous;
            if (st.key == tst->key)
            {
                st.repetition = static_cast<int8_t>(tst->repetition + 1);
                break;
            }
        }
    }
}

void Position::do_move(const Move move)
{
    State st;
    make_move(move, st);
    ThreadPool::states.push_back(st);
    state = &ThreadPool::states.back();
}

void Position::unmake_move(const Move& move)
{
    side_to_move = !side_to_move;
    const uint8_t from = move.from();
    const uint8_t to = move.to();
    const auto flag = move.flag();

    if (flag >= MoveFlag::knight_promotion)
    {
        remove_piece(to);
        put_piece(side_to_move == white ? P : p, to);
    }
    else if (flag == MoveFlag::king_castle)
    {
        move_piece(to - 1, to + 1);
    }
    else if (flag == MoveFlag::queen_castle)
    {
        move_piece(to + 1, to - 2);
    }

    move_piece(to, from);
    if (state->captured_piece != nil)
    {
        uint8_t captured_square = to;
        if (flag == MoveFlag::ep_capture)
        {
            captured_square -= Delta<Direction::Up>(side_to_move);
        }

        put_piece(state->captured_piece, captured_square);
    }

    state = state->previous;
}

void Position::make_null_move(State& st)
{
    std::memcpy(&st, state, sizeof(State));

    st.previous = state;
    st.repetition = 1;
    st.ply = 0;
    st.key ^= Zobrist::side_key;

    if (st.en_passant_square != -1)
    {
        st.key ^= Zobrist::en_passant_key[st.en_passant_square % 8];
        st.en_passant_square = -1;
    }

    side_to_move = !side_to_move;
    st.attacks = UINT64_MAX;
    st.pinned = UINT64_MAX;
    state = &st;
}

void Position::unmake_null_move()
{
    state = state->previous;
    side_to_move = !side_to_move;
}

bool Position::is_pseudo_legal(const Move move) const
{
    const auto flag = move.flag();
    const auto from = static_cast<int8_t>(move.from());
    const auto to = static_cast<int8_t>(move.to());
    const Piece moved_piece = piece_on[from];

    if (moved_piece == nil || color_of(moved_piece) != side_to_move) return false;
    if ((flag == MoveFlag::capture && piece_on[to] == nil) || (flag != MoveFlag::capture && piece_on[to] != nil))
        return
            false;
    if (moved_piece != P && moved_piece != p)
    {
        if (moved_piece != K && moved_piece != k)
        {
            if (!(flag == MoveFlag::quiet_move || flag == MoveFlag::capture)) return false;
        }
        else if (!(flag == MoveFlag::quiet_move || flag == MoveFlag::capture || flag == MoveFlag::king_castle || flag ==
            MoveFlag::queen_castle))
            return false;
    }
    if (std::to_underlying(flag) == 6 || std::to_underlying(flag) == 7) return false;

    const uint64_t to_board = 1ull << to;
    if (occupations[side_to_move] & to_board) return false;

    if (state->pinned & (1ull << from))
    {
        if (const auto king_board = boards[side_to_move == white ? K : k];
            !(lines_intersect[from][to] & king_board))
        {
            return false;
        }
    }

    static constexpr uint64_t rank18 = 0xFF000000000000FFull;
    if (type_of(moved_piece) == Pawn)
    {
        if (const bool is_single_push = (from + Delta<Direction::Up>(side_to_move) == to) && piece_on[to] == nil;
            is_single_push)
        {
            const bool is_promo = rank18 & to_board;
            if (is_promo && (flag < MoveFlag::knight_promotion || flag > MoveFlag::queen_promotion)) return false;
            if (!is_promo && flag != MoveFlag::quiet_move) return false;
        }
        else if (const bool is_double_push = (from + 2 * Delta<Direction::Up>(side_to_move) == to)
            && piece_on[to] == nil
            && piece_on[to - Delta<Direction::Up>(side_to_move)] == nil
            && (side_to_move == white ? from / 8 == 6 : from / 8 == 1); is_double_push)
        {
            if (flag != MoveFlag::double_push) return false;
        }
        else if (const bool is_normal_capture = pawn_attack_tables[side_to_move][from] & occupations[!side_to_move] &
            to_board; is_normal_capture)
        {
            const bool is_promo = rank18 & to_board;
            if (is_promo && (flag < MoveFlag::knight_promo_capture || flag > MoveFlag::queen_promo_capture))
                return
                    false;
            if (!is_promo && flag != MoveFlag::capture) return false;
        }
        else if (const bool is_ep = flag == MoveFlag::ep_capture
            && to == state->en_passant_square
            && (pawn_attack_tables[side_to_move][from] & to_board); is_ep)
        {
            if (flag != MoveFlag::ep_capture) return false;
        }
        else
        {
            return false;
        }
    }
    else if (type_of(moved_piece) == Knight)
    {
        if (!(knight_attack_tables[from] & to_board & ~occupations[side_to_move])) return false;
    }
    else if (type_of(moved_piece) == Rook)
    {
        if (!(get_rook_attack(from, occupations[2]) & to_board & ~occupations[side_to_move])) return false;
    }
    else if (type_of(moved_piece) == Bishop)
    {
        if (!(get_bishop_attack(from, occupations[2]) & to_board & ~occupations[side_to_move])) return false;
    }
    else if (type_of(moved_piece) == Queen)
    {
        if (!((get_bishop_attack(from, occupations[2]) | get_rook_attack(from, occupations[2])) & to_board & ~
            occupations[side_to_move]))
            return false;
    }
    else if (type_of(moved_piece) == King)
    {
        if (flag == MoveFlag::king_castle || flag == MoveFlag::queen_castle)
        {
            if (state->checker) return false;

            const uint64_t king_path = side_to_move == white ? 0x6000000000000000ull : 0x60ull;
            const uint64_t queen_path = side_to_move == white ? 0xE00000000000000ull : 0xEull;
            const uint64_t queen_check_path = side_to_move == white ? 0xC00000000000000ull : 0xCull;

            if (flag == MoveFlag::king_castle)
            {
                if (to != from + 2) return false;
                if (!(state->castling_rights & (side_to_move == white ? white_king : black_king))) return false;
                if (occupations[2] & king_path) return false;
                if (state->attacks & king_path) return false;
            }
            else
            {
                if (to != from - 2) return false;
                if (!(state->castling_rights & (side_to_move == white ? white_queen : black_queen))) return false;
                if (occupations[2] & queen_path) return false;
                if (state->attacks & queen_check_path) return false;
            }
            return true;
        }

        if (!(king_attack_tables[from] & to_board & ~occupations[side_to_move])) return false;
        if (to_board & state->attacks) return false;
    }

    if (state->checker)
    {
        if (moved_piece != K && moved_piece != k)
        {
            if (std::popcount(state->checker) != 1)
            {
                return false;
            }
            if (flag == MoveFlag::ep_capture)
            {
                const int ep_capture_sq = to + Delta<Direction::Up>(side_to_move);
                return state->check_blocker & (1ull << ep_capture_sq) || (state->check_blocker & to_board);
            }

            if (!(state->check_blocker & to_board)) return false;
        }
    }

    return true;
}

bool Position::is_quiet(const Move move) const
{
    const auto flag = move.flag();
    return piece_on[move.to()] == nil && flag < MoveFlag::knight_promotion && flag != MoveFlag::ep_capture;
}

bool Position::is_legal(const Move move) const
{
    if (move.flag() == MoveFlag::ep_capture)
    {
        const bool us = side_to_move;
        const auto to = move.to();
        const auto king_index = lsb(boards[us == white ? K : k]);
        const uint64_t eq = boards[us == white ? q : Q];
        const uint64_t eb = boards[us == white ? b : B];
        const uint64_t er = boards[us == white ? r : R];
        const uint64_t occ = (occupations[2] ^ (1ull << (to + (us == white ? 8 : -8))) ^ (1ull << move.from())) | (1ull
            << to);

        return !((get_bishop_attack(king_index, occ) & (eq | eb)) | (get_rook_attack(king_index, occ) & (eq | er)));
    }
    return true;
}

template <const bool side>
[[nodiscard]] uint64_t Position::get_checker_of() const
{
    static constexpr uint8_t king = side == white ? K : k;
    static constexpr uint8_t rook = side == white ? r : R;
    static constexpr uint8_t bishop = side == white ? b : B;
    static constexpr uint8_t queen = side == white ? q : Q;
    static constexpr uint8_t pawn = side == white ? p : P;
    static constexpr uint8_t knight = side == white ? n : N;

    const uint8_t king_index = lsb(boards[king]);

    const uint64_t checkers = (pawn_attack_tables[side][king_index] & (boards[pawn]))
        | (knight_attack_tables[king_index] & boards[knight])
        | (get_bishop_attack(king_index, occupations[2]) & (boards[bishop] | boards[queen]))
        | (get_rook_attack(king_index, occupations[2]) & (boards[rook] | boards[queen]));
    return checkers;
}

template <const bool side>
[[nodiscard]] uint64_t Position::get_check_blocker_of() const
{
    return state->checker | lines_between[lsb(state->checker)][lsb(side == white ? boards[K] : boards[k])];
}

template <const bool side>
[[nodiscard]] uint64_t Position::get_attacked_map_of() const
{
    static constexpr int enemy_rook = side == white ? r : R;
    static constexpr int enemy_bishop = side == white ? b : B;
    static constexpr int enemy_queen = side == white ? q : Q;
    static constexpr int enemy_pawn = side == white ? p : P;
    static constexpr int enemy_knight = side == white ? n : N;
    static constexpr int enemy_king = side == white ? k : K;
    static constexpr int our_king = side == white ? K : k;

    const uint64_t enemy_queen_board = boards[enemy_queen];
    const uint64_t occ = occupations[2] ^ boards[our_king];

    uint64_t attacks = 0;

    auto enemy_rook_board = enemy_queen_board | boards[enemy_rook];
    while (enemy_rook_board)
    {
        attacks |= get_rook_attack(pop_lsb(enemy_rook_board), occ);
    }

    auto enemy_bishop_board = enemy_queen_board | boards[enemy_bishop];
    while (enemy_bishop_board)
    {
        attacks |= get_bishop_attack(pop_lsb(enemy_bishop_board), occ);
    }

    auto enemy_knight_board = boards[enemy_knight];
    while (enemy_knight_board)
    {
        attacks |= knight_attack_tables[pop_lsb(enemy_knight_board)];
    }

    auto enemy_pawn_board = boards[enemy_pawn];
    while (enemy_pawn_board)
    {
        attacks |= pawn_attack_tables[!side][pop_lsb(enemy_pawn_board)];
    }

    return attacks | king_attack_tables[lsb(boards[enemy_king])];
}

template <const bool side>
[[nodiscard]] uint64_t Position::get_pinned_board_of() const
{
    uint64_t attacker = 0, pinned_board = 0;
    static constexpr uint8_t king = side == white ? K : k;
    static constexpr uint8_t rook = side == white ? r : R;
    static constexpr uint8_t bishop = side == white ? b : B;
    static constexpr uint8_t queen = side == white ? q : Q;

    const int king_index = lsb(boards[king]);

    attacker = (get_rook_attack(king_index, occupations[!side]) & (boards[rook] | boards[queen]))
        | (get_bishop_attack(king_index, occupations[!side]) & (boards[bishop] | boards[queen]));

    while (attacker)
    {
        const int sniper = pop_lsb(attacker);

        if (const uint64_t between = lines_between[sniper][king_index] & occupations[side];
            std::has_single_bit(between))
            pinned_board |= between;
    }

    return pinned_board;
}

void Position::construct_zobrist_key() const
{
    state->key = 0;
    state->pawn_key = 0;
    state->major_key = 0;
    state->non_pawn_keys = {0, 0};
    for (int i = 0; i < 14; i++)
    {
        auto board = boards[i];
        while (board)
        {
            const auto piece = static_cast<Piece>(i);
            const auto sq = pop_lsb(board);
            state->key ^= Zobrist::piece_keys[i][sq];
            if (type_of(piece) == Pawn)
            {
                state->pawn_key ^= Zobrist::piece_keys[piece][sq];
            }
            else
            {
                state->non_pawn_keys[color_of(piece)] ^= Zobrist::piece_keys[piece][sq];

                if (type_of(piece) >= Rook)
                {
                    state->major_key ^= Zobrist::piece_keys[piece][sq];
                }
            }
        }
    }

    if (state->en_passant_square != -1)
    {
        state->key ^= Zobrist::en_passant_key[state->en_passant_square % 8];
    }

    if (side_to_move == black)
    {
        state->key ^= Zobrist::side_key;
    }

    state->key ^= Zobrist::castling_keys[state->castling_rights];
}

void Position::get_pinned() const
{
    state->pinned = side_to_move == white ? get_pinned_board_of<white>() : get_pinned_board_of<black>();
}

void Position::get_attacks() const
{
    state->attacks = side_to_move == white ? get_attacked_map_of<white>() : get_attacked_map_of<black>();
}

void Position::fill_info() const
{
    if (state->attacks == UINT64_MAX)
    {
        get_attacks();
        get_pinned();
    }
}

void Position::get_checks() const
{
    if (!side_to_move)
    {
        state->checker = get_checker_of<white>();
        if (state->checker)
        {
            state->check_blocker = get_check_blocker_of<white>();
        }
    }
    else
    {
        state->checker = get_checker_of<black>();
        if (state->checker)
        {
            state->check_blocker = get_check_blocker_of<black>();
        }
    }
}

bool Position::upcoming_repetition(const int ply) const
{
    const auto end = std::min(static_cast<uint16_t>(state->rule_50), state->ply);

    if (end < 3) return false;

    const uint64_t original_key = state->key;
    const State* prev = state->previous;
    uint64_t other = original_key ^ prev->key ^ Zobrist::side_key;

    for (int i = 3; i <= end; i += 2)
    {
        prev = prev->previous;
        other ^= prev->key ^ prev->previous->key ^ Zobrist::side_key;
        prev = prev->previous;

        if (other != 0) continue;

        const uint64_t diff = original_key ^ prev->key;
        uint64_t slot = Cuckoo::hash1(diff);

        if (diff != Cuckoo::cuckoo_key[slot]) slot = Cuckoo::hash2(diff);
        if (diff != Cuckoo::cuckoo_key[slot]) continue;

        if (const Move move = Cuckoo::cuckoo_move[slot]; !(occupations[2] & lines_between[move.from()][move.to()]))
        {
            if (ply > i) return true;

            if (prev->repetition > 1) return true;
        }
    }

    return false;
}

void Position::print_board() const
{
    std::string lines[8];
    for (int rank = 0; rank < 8; rank++)
    {
        lines[rank] += std::to_string(8 - rank) + "  ";
        for (int file = 0; file < 8; file++)
        {
            lines[rank] += ". ";
        }
    }
    for (int _pos = 0; _pos < 64; _pos++)
    {
        lines[_pos / 8][3 + _pos % 8 * 2] = piece_icons[piece_on[_pos]];
    }

    std::println("   a b c d e f g h");
    for (const auto& i : lines) std::println("{}", i);
    std::println("FEN: {}", to_fen());
}

std::string Position::to_fen() const
{
    std::stringstream fen;
    for (int row = 0; row < 8; row++)
    {
        int empty = 0;
        for (int col = 0; col < 8; col++)
        {
            if (piece_on[row * 8 + col] != nil)
            {
                if (empty)
                {
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
    else
    {
        static constexpr char rights[] = {'K', 'Q', 'k', 'q'};
        for (int i = 0; i < 4; i++)
        {
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
