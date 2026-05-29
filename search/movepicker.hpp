#pragma once

#include <cstdint>

#include "history.hpp"
#include "../position/movegen.hpp"
#include "../position/move.hpp"
#include "see.hpp"
#include "thread.hpp"

static constexpr int16_t mvv[12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605
};

enum class Stage: uint8_t
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
    SearchThread& thread;
    SearchEntry* ss;
    Move pv = null_move;
    Move* non_captures_start;
    Move* current;
    Move* bad_captures_end;
    MoveList moves;
    int threshold;
    int scores[256];
    bool noisy_only;
    Stage stage = Stage::generating_capture_moves;

    explicit MovePicker(SearchThread& _thread, const bool _noisy_only, const Move _pv, SearchEntry* _ss,
                        const int _threshold = 0) : thread(_thread), ss(_ss), noisy_only(_noisy_only)
    {
        bad_captures_end = &moves.list[255];
        threshold = _threshold;
        if (thread.position.state->attacks == UINT64_MAX)
        {
            thread.position.get_attacks();
            thread.position.get_pinned();
        }

        if (const auto pv_flag = pv.flag(); _pv && thread.position.is_pseudo_legal(_pv)
            && !(noisy_only
                && pv_flag != queen_promotion
                && pv_flag != capture
                && pv_flag < knight_promo_capture
                && pv_flag != ep_capture))
        {
            pv = _pv;
            stage = Stage::TT_moves;
        }
    }

    std::pair<Move, int> pick()
    {
        const auto& pos = thread.position;
        const auto& history = thread.history;

        switch (stage)
        {
        case Stage::TT_moves:
            stage = Stage::generating_capture_moves;
            return {pv, 0};
        case Stage::generating_capture_moves:
            stage = Stage::good_capture_moves;
            pseudo_legals<noisy>(pos, moves);
            score_mvv_caphist(moves.begin(), moves.last);
            non_captures_start = moves.last;
            current = moves.begin();
            [[fallthrough]];
        case Stage::good_capture_moves:
            while (current < non_captures_start)
            {
                select_highest(current, non_captures_start);
                Move move = *current;
                auto score = scores[current - moves.begin()];
                current++;

                if (move == pv) continue; // Skip TT move

                if (static_exchange_evaluation(thread.position, move) < threshold)
                {
                    *bad_captures_end-- = move;
                    continue;
                }

                return {move, score};
            }
            stage = Stage::killer_1;
            [[fallthrough]];
        case Stage::killer_1:
            stage = Stage::killer_2;
            if (const Move killer = history.killers.get(ss->plies, 0); killer != pv && thread.position.is_pseudo_legal(killer))
            {
                return {killer, UINT16_MAX};
            }
            [[fallthrough]];
        case Stage::killer_2:
            stage = Stage::generating_quiet_moves;
            if (const Move killer = history.killers.get(ss->plies, 1); killer != pv && thread.position.is_pseudo_legal(killer))
            {
                return {killer, UINT16_MAX};
            }
            [[fallthrough]];
        case Stage::generating_quiet_moves:
            if (!noisy_only)
            {
                pseudo_legals<quiet>(thread.position, moves);
                score_history(non_captures_start, moves.last);
                stage = Stage::quiet_moves;
                current = non_captures_start;
            }
            else
            {
                stage = Stage::none;
                return {null_move, 0};
            }
            [[fallthrough]];
        case Stage::quiet_moves:
            while (current < moves.last)
            {
                select_highest(current, moves.last);
                Move move = *current;
                auto score = scores[current - moves.begin()];
                current++;

                if (move == pv ||
                    move == history.killers.get(ss->plies, 0) ||
                    move == history.killers.get(ss->plies, 1))
                {
                    continue;
                }

                return {move, score};
            }

            stage = Stage::bad_capture_moves;
            current = &moves.list[255];
            [[fallthrough]];
        case Stage::bad_capture_moves:
            while (current > bad_captures_end)
            {
                Move move = *current;
                current--;
                return {move, -1};
            }
            stage = Stage::none;
            [[fallthrough]];
        case Stage::none: default:
            return {null_move, 0};
        }
    }

    std::pair<Move, int> next_move()
    {
        auto move = pick();
        if (!move.first) return {null_move, 0};
        const auto& pos = thread.position;

        while (!pos.is_legal(move.first))
        {
            move = pick();
            if (!move.first) return {null_move, 0};
        }
        return move;
    }

    void score_mvv_caphist(const Move* begin, const Move* end)
    {
        if (begin == end) return;

        const auto& pos = thread.position;
        const auto& history = thread.history;
        const bool stm = pos.side_to_move;

        for (const Move* move = begin; move < end; ++move)
        {
            const auto index = move - begin;
            const auto from = move->from();
            const auto to = move->to();

            int moved = pos.piece_on[from];
            int captured = pos.piece_on[to];

            if (captured == nil) captured = P;
            else if (captured >= 6) captured -= 6;
            if (moved >= 6) moved -= 6;

            scores[index] = mvv[captured] * mvv_weight() / 1024 + history.capture.table[stm][moved][captured][to] *
                capture_history_weight() / 1024;
            if (move->flag() >= knight_promo_capture)
                scores[index] = scores[index]
                    + value_of(move->promoted_to());
        }
    }

    void score_history(const Move* begin, const Move* end)
    {
        if (begin == end) return;

        const auto offset = non_captures_start - moves.begin();
        const auto prev = (ss - 1)->piece_to != UINT16_MAX ? (ss - 1)->piece_to : 0;
        const auto prev2 = (ss - 2)->piece_to != UINT16_MAX ? (ss - 2)->piece_to : 0;
        const auto prev4 = (ss - 4)->piece_to != UINT16_MAX ? (ss - 4)->piece_to : 0;
        const auto& pos = thread.position;
        const auto& history = thread.history;

        for (const Move* move = begin; move < end; ++move)
        {
            const auto index = offset + (move - begin);
            const auto from = move->from();
            const auto to = move->to();
            int moved = pos.piece_on[from];
            if (moved >= 6) moved -= 6;
            scores[index] =
                (history.butterfly_history.table[pos.side_to_move][from][to]
                    + history.piece_to_history.table[pos.side_to_move][moved][to]) * piece_history_weight() / 1024
                + history.continuation.counter_moves[pos.side_to_move][prev >> 6][prev & 0b111111][moved][to] *
                counter_move_weight() / 1024
                + history.continuation.follow_up[pos.side_to_move][prev2 >> 6][prev2 & 0b111111][moved][to] *
                follow_up_weight() / 1024
                + history.continuation.four_plies[pos.side_to_move][prev4 >> 6][prev4 & 0b111111][moved][to] *
                four_plies_weight() / 1024;
        }
    }

    void select_highest(Move* start, const Move* end)
    {
        Move* best = start;
        auto best_score = scores[start - moves.begin()];

        for (Move* it = start + 1; it < end; ++it)
        {
            if (const auto score = scores[it - moves.begin()]; score > best_score)
            {
                best_score = score;
                best = it;
            }
        }

        std::swap(*start, *best);
        std::swap(scores[start - moves.begin()], scores[best - moves.begin()]);
    }
};
