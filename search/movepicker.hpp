#pragma once

#include <cstdint>

#include "history.hpp"
#include "../position/movegen.hpp"
#include "../position/move.hpp"
#include "see.hpp"

static constexpr int16_t mvv[12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605
};

enum Stages: uint8_t
{
    TT_moves,
    generating_capture_moves,
    good_capture_moves,
    killer_1,
    killer_2,
    generating_quiet_moves,
    quiet_moves,
    bad_capture_moves,
    none,
};

struct MovePicker
{
    Position* pos;
    SearchEntry* ss;
    Move pv = null_move;
    Move* non_captures_start;
    Move* current;
    Move* bad_captures_end;
    MoveList moves;
    int threshold;
    int16_t scores[256];
    bool noisy_only;
    Stages stage = generating_capture_moves;

    explicit MovePicker(Position* _pos, const bool _noisy_only, const Move _pv, SearchEntry* _ss,
                        const int _threshold = 0) : pos(_pos), ss(_ss), noisy_only(_noisy_only)
    {
        bad_captures_end = &moves.list[255];
        threshold = _threshold;
        if (const auto pv_flag = pv.flag(); _pv && pos->is_pseudo_legal(_pv)
            && !(noisy_only
                && pv_flag != queen_promotion
                && pv_flag != capture
                && pv_flag < knight_promo_capture
                && pv_flag != ep_capture))
        {
            bool is_valid = true;

            if (pos->state->checker)
            {
                if (pos->piece_on[_pv.from()] != K && pos->piece_on[_pv.from()] != k)
                {
                    if (std::popcount(pos->state->checker) > 1)
                    {
                        is_valid = false;
                    }
                    else if (_pv.flag() == ep_capture)
                    {
                        const int ep_capture_sq = _pv.to() + Delta<Up>(pos->side_to_move);
                        is_valid = pos->state->check_blocker & (1ull << ep_capture_sq);
                    }
                    else
                    {
                        is_valid = pos->state->check_blocker & (1ull << _pv.to());
                    }
                }
            }

            if (is_valid)
            {
                pv = _pv;
                stage = TT_moves;
            }
        }
    }

    std::pair<Move, int16_t> pick()
    {
        switch (stage)
        {
        case TT_moves:
            stage = generating_capture_moves;
            return {pv, 0};
        case generating_capture_moves:
            stage = good_capture_moves;
            pseudo_legals<noisy>(*pos, moves);
            score_mvv_caphist(moves.begin(), moves.last);
            non_captures_start = moves.last;
            current = moves.begin();
            [[fallthrough]];
        case good_capture_moves:
            while (current < non_captures_start)
            {
                select_highest(current, non_captures_start);
                Move move = *current;
                int16_t score = scores[current - moves.begin()];
                current++;

                if (move == pv) continue; // Skip TT move

                if (static_exchange_evaluation(*pos, move) < threshold)
                {
                    *bad_captures_end-- = move;
                    continue;
                }

                return {move, score};
            }
            stage = killer_1;
            [[fallthrough]];
        case killer_1:
            stage = killer_2;
            if (const Move killer = Killers::get(ss->plies, 0); killer != pv && pos->is_pseudo_legal(killer))
            {
                return {killer, UINT16_MAX};
            }
            [[fallthrough]];
        case killer_2:
            stage = generating_quiet_moves;
            if (const Move killer = Killers::get(ss->plies, 1); killer != pv && pos->is_pseudo_legal(killer))
            {
                return {killer, UINT16_MAX};
            }
            [[fallthrough]];
        case generating_quiet_moves:
            if (!noisy_only)
            {
                pseudo_legals<quiet>(*pos, moves);
                score_history(non_captures_start, moves.last);
                stage = quiet_moves;
                current = non_captures_start;
            }
            else
            {
                stage = none;
                return {null_move, 0};
            }
            [[fallthrough]];
        case quiet_moves:
            while (current < moves.last)
            {
                select_highest(current, moves.last);
                Move move = *current;
                int16_t score = scores[current - moves.begin()];
                current++;

                if (move == pv ||
                    move == Killers::get(ss->plies, 0) ||
                    move == Killers::get(ss->plies, 1))
                {
                    continue;
                }

                return {move, score};
            }

            stage = bad_capture_moves;
            current = &moves.list[255];
            [[fallthrough]];
        case bad_capture_moves:
            while (current > bad_captures_end)
            {
                Move move = *current;
                current--;
                return {move, -1};
            }
            stage = none;
            [[fallthrough]];
        case none: default:
            return {null_move, 0};
        }
    }

    std::pair<Move, int16_t> next_move()
    {
        auto move = pick();
        if (!move.first) return {null_move, 0};
        while (!check_move_legality(*pos, move.first))
        {
            move = pick();
            if (!move.first) return {null_move, 0};
        }
        return move;
    }

    void score_mvv_caphist(const Move* begin, const Move* end)
    {
        if (begin == end) return;

        const bool stm = pos->side_to_move;

        for (const Move* move = begin; move < end; ++move)
        {
            const auto index = move - begin;
            const auto from = move->from();
            const auto to = move->to();

            int moved = pos->piece_on[from];
            int captured = pos->piece_on[to];

            if (captured == nil) captured = P;
            else if (captured >= 6) captured -= 6;
            if (moved >= 6) moved -= 6;

            scores[index] = static_cast<int16_t>(mvv[captured] + Capture::table[stm][moved][captured][to]);
            if (move->flag() >= knight_promo_capture)
                scores[index] = static_cast<int16_t>(scores[index]
                    + value_of(move->promoted_to<false>()));
        }
    }

    void score_history(const Move* begin, const Move* end)
    {
        if (begin == end) return;

        const auto offset = non_captures_start - moves.begin();
        const auto prev = (ss - 1)->piece_to != UINT16_MAX ? (ss - 1)->piece_to : 0;
        const auto prev2 = (ss - 2)->piece_to != UINT16_MAX ? (ss - 2)->piece_to : 0;

        for (const Move* move = begin; move < end; ++move)
        {
            const auto index = offset + (move - begin);
            const auto from = move->from();
            const auto to = move->to();
            int moved = pos->piece_on[from];
            if (moved >= 6) moved -= 6;
            scores[index] = static_cast<int16_t>(
                (ButterflyHistory::table[pos->side_to_move][from][to]
                    + PieceToHistory::table[pos->side_to_move][moved][to]) / 2
                + Continuation::counter_moves[pos->side_to_move][prev >> 6][prev & 0b111111][moved][to]
                + Continuation::follow_up[pos->side_to_move][prev2 >> 6][prev2 & 0b111111][moved][to]);
        }
    }

    void select_highest(Move* start, const Move* end)
    {
        Move* best = start;
        int16_t best_score = scores[start - moves.begin()];

        for (Move* it = start + 1; it < end; ++it)
        {
            if (const int16_t score = scores[it - moves.begin()]; score > best_score)
            {
                best_score = score;
                best = it;
            }
        }

        std::swap(*start, *best);
        std::swap(scores[start - moves.begin()], scores[best - moves.begin()]);
    }
};
