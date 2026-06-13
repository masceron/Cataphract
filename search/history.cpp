#include <cstring>

#include "history.hpp"
#include "params.hpp"
#include "../position/position.hpp"

void Killers::insert(const Move move, const int ply) const
{
    auto killers_of_ply = table[ply];
    if (move == killers_of_ply[0] || move == killers_of_ply[1]) return;
    killers_of_ply[1] = killers_of_ply[0];
    killers_of_ply[0] = move;
}

[[nodiscard]] Move Killers::get(const int ply, const int order) const
{
    return table[ply][order];
}

CaptureEntry::CaptureEntry(const uint8_t _moved, const uint8_t _captured, const Move move) :
    moved(_moved & 7), captured(_captured != nil ? _captured & 7 : P), sq(move.to())
{
}

void Capture::apply(const bool stm, const uint8_t moved_piece, const uint8_t captured_piece, const int sq,
                    const int16_t bonus)
{
    const int16_t clamped_bonus = std::clamp(bonus, static_cast<int16_t>(-max_capture_history()),
                                             max_capture_history());
    table[stm][moved_piece][captured_piece][sq] += clamped_bonus - table[stm][moved_piece][captured_piece][sq] *
        abs(clamped_bonus) / max_capture_history();
}

void Capture::update(const std::forward_list<CaptureEntry>& searched, const bool stm, uint8_t moved_piece,
                     uint8_t captured_piece, const uint8_t sq, const uint8_t depth)
{
    moved_piece &= 7;
    captured_piece = captured_piece == nil ? P : captured_piece & 7;

    const auto bonus = static_cast<int16_t>(capture_history_bonus() * depth);
    apply(stm, moved_piece, captured_piece, sq, bonus);

    for (const auto& [_moved, _captured, _sq] : searched)
    {
        apply(stm, _moved, _captured, _sq, static_cast<int16_t>(-bonus));
    }
}

void ButterflyHistory::apply(const bool side, const int from, const int to, const int16_t bonus)
{
    const int16_t clamped_bonus = std::clamp(bonus, static_cast<int16_t>(-max_butterfly_history()),
                                             max_butterfly_history());
    table[side][from][to] += clamped_bonus - table[side][from][to] * abs(clamped_bonus) / max_butterfly_history();
}

void ButterflyHistory::update(const std::forward_list<Move>& searched, const bool side, const int from, const int to,
                              const uint8_t depth)
{
    const auto bonus = static_cast<int16_t>(butterfly_history_scale() * depth - butterfly_history_minus());
    apply(side, from, to, bonus);

    for (const Move move : searched)
    {
        apply(side, move.from(), move.to(), static_cast<int16_t>(-bonus));
    }
}

void PieceToHistory::apply(const bool side, const Piece piece, const int to, const int16_t bonus)
{
    const int16_t clamped_bonus = std::clamp(bonus, static_cast<int16_t>(-max_piece_to_history()),
                                             max_piece_to_history());
    table[side][piece][to] += clamped_bonus - table[side][piece][to] * abs(clamped_bonus) / max_piece_to_history();
}

void PieceToHistory::update(const Position& pos, const std::forward_list<Move>& searched, const bool side, Piece piece,
                            const int to, const uint8_t depth)
{
    piece = static_cast<Piece>(piece & 7);
    const auto bonus = static_cast<int16_t>(piece_to_history_scale() * depth - piece_to_history_minus());
    apply(side, piece, to, bonus);

    for (const Move move : searched)
    {
        uint8_t p = pos.piece_on[move.from()] & 7;

        apply(side, static_cast<Piece>(p), move.to(), static_cast<int16_t>(-bonus));
    }
}

void Continuation::apply(std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 6>, 64>, 6>, 2>& table,
                         const bool stm, const uint8_t prev_piece, const uint8_t prev_to,
                         const uint8_t piece,
                         const uint8_t to, const int16_t bonus)
{
    const int16_t clamped_bonus = std::clamp(bonus, static_cast<int16_t>(-max_continuation_history()),
                                             max_continuation_history());

    table[stm][prev_piece][prev_to][piece][to] += clamped_bonus - table[stm][prev_piece][prev_to][piece][to] *
        abs(clamped_bonus) / max_continuation_history();
}

void Continuation::update(const Position& pos, const std::forward_list<Move>& searched, const Move move,
                          const uint8_t depth, const SearchEntry* ss)
{
    const auto bonus = static_cast<int16_t>(continuation_history_scale() * depth - continuation_history_minus());
    const bool stm = pos.side_to_move;
    const uint8_t piece = pos.piece_on[move.from()] & 7;

    const auto offset_to_idx = [](const int offset)
    {
        return offset == 1 ? 0 : offset == 2 ? 1 : 2;
    };

    for (const int offset : {1, 2, 4})
    {
        const auto prev = ss - offset;
        if (prev->piece_to == UINT16_MAX) continue;

        const uint8_t prev_piece = prev->piece_to >> 6 & 7;
        const uint8_t prev_to = prev->piece_to & 0b111111;
        const auto idx = offset_to_idx(offset);

        apply(continuation_table[idx], stm, prev_piece, prev_to, piece, move.to(), bonus);

        for (const auto& tpm : searched)
        {
            const uint8_t tmp = pos.piece_on[tpm.from()] & 7;
            apply(continuation_table[idx], stm, prev_piece, prev_to, tmp, tpm.to(), -bonus);
        }
    }
}

uint16_t Corrections::index_of(const uint64_t key, const uint16_t size)
{
    return key & (size - 1);
}

void Corrections::apply(int16_t& entry, const int bonus)
{
    entry += bonus - entry * std::abs(bonus) / correction_limit;
}

void Corrections::update(const int delta, const Position& pos, const uint8_t depth)
{
    auto& pawns = pawn_corrections[pos.side_to_move][index_of(pos.state->pawn_key)];
    auto& non_pawns_white = white_non_pawns[pos.side_to_move][index_of(pos.state->non_pawn_keys[white])];
    auto& non_pawns_black = black_non_pawns[pos.side_to_move][index_of(pos.state->non_pawn_keys[black])];
    auto& major = major_piece_corrections[pos.side_to_move][index_of(pos.state->major_key)];

    const auto continuation_correct = [&](const int offset, const int bonus)
    {
        if (pos.state->ply < offset) return;
        const auto key = pos.state->key;
        const State* state = pos.state;
        for (int i = 0; i < offset; i++)
        {
            state = state->previous;
        }
        apply(continuation_corrections[index_of(key ^ state->key, continuation_correction_size)], bonus);
    };

    const int bonus = std::clamp(delta * depth / 8, -correction_limit / 4, correction_limit / 4);

    apply(pawns, bonus);
    apply(non_pawns_white, bonus);
    apply(non_pawns_black, bonus);
    apply(major, bonus);
    continuation_correct(1, bonus);
    continuation_correct(2, bonus);
    continuation_correct(4, bonus);
}

[[nodiscard]] int Corrections::correct(int static_eval, const Position& pos) const
{
    const auto pawn_correction = pawn_corrections[pos.side_to_move][index_of(pos.state->pawn_key)];
    const auto non_pawns_white_correction = white_non_pawns[pos.side_to_move][
        index_of(pos.state->non_pawn_keys[white])];
    const auto non_pawns_black_correction = black_non_pawns[pos.side_to_move][
        index_of(pos.state->non_pawn_keys[black])];
    const auto major_correction = major_piece_corrections[pos.side_to_move][index_of(pos.state->major_key)];

    const auto continuation_correct = [&]()
    {
        const auto key = pos.state->key;
        int corr = 0;
        if (auto& state = pos.state; state->ply >= 1)
        {
            corr += continuation_corrections[index_of(key ^ state->previous->key, continuation_correction_size)]
                * continuation_correction_weight_1();
            if (state->ply >= 2)
            {
                corr += continuation_corrections[index_of(key ^ state->previous->previous->key,
                                                          continuation_correction_size)]
                    * continuation_correction_weight_2();

                if (state->ply >= 4)
                {
                    corr += continuation_corrections[index_of(key ^ state->previous->previous->previous->previous->key,
                                                              continuation_correction_size)]
                        * continuation_correction_weight_4();
                }
            }
        }

        return corr;
    };

    const int corrections =
        pawn_correction * pawn_correction_weight() +
        non_pawns_white_correction * non_pawn_correction_weight() +
        non_pawns_black_correction * non_pawn_correction_weight() +
        major_correction * major_correction_weight() +
        continuation_correct();

    static_eval += corrections / 1024;

    return std::clamp(static_eval, mated_in_max_ply + 1, mate_in_max_ply - 1);
}

void History::update_quiet_histories(const Position& pos, const int depth, const Move picked_move,
                                     const SearchEntry* ss,
                                     const std::forward_list<Move>& quiets_searched)
{
    killers.insert(picked_move, ss->plies);
    butterfly_history.update(quiets_searched, pos.side_to_move, picked_move.from(), picked_move.to(),
                             depth);
    piece_to_history.update(pos, quiets_searched, pos.side_to_move, pos.piece_on[picked_move.from()],
                            picked_move.to(),
                            depth);
    continuation.update(pos, quiets_searched, picked_move, depth, ss);
}

void History::clear()
{
    memset(this, 0, sizeof(History));
}
