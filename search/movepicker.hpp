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
    bool non_quiet_only;
    Stages stage = generating_capture_moves;

    explicit MovePicker(Position* _pos, const bool _capture_only, const Move _pv, SearchEntry* _ss,
                        const int _threshold = 0) : pos(_pos), ss(_ss), non_quiet_only(_capture_only)
    {
        bad_captures_end = &moves.list[255];
        threshold = _threshold;
        if (_pv && pos->is_pseudo_legal(_pv)
            && !(non_quiet_only
                && _pv.flag() != queen_promotion
                && _pv.flag() != capture
                && _pv.flag() < knight_promo_capture
                && _pv.flag() != ep_capture))
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
            sort_mvv_capthist(moves.begin(), moves.last);
            non_captures_start = moves.last;
            current = moves.begin() - 1;
            [[fallthrough]];
        case good_capture_moves:
            current++;
            if (*current == pv) current++;
            if (current >= non_captures_start)
            {
                stage = generating_quiet_moves;
            }
            else
            {
                while (current < non_captures_start && (*current == pv || static_exchange_evaluation(*pos, *current) <
                    threshold))
                {
                    if (*current != pv)
                    {
                        *bad_captures_end-- = *current;
                    }
                    current++;
                }

                if (current < non_captures_start)
                {
                    return {*current, scores[current - moves.begin()]};
                }
                stage = generating_quiet_moves;
            }
            [[fallthrough]];
        case generating_quiet_moves:
            if (!non_quiet_only)
            {
                pseudo_legals<quiet>(*pos, moves);
                sort_history(non_captures_start, moves.last);
                stage = quiet_moves;
                current = non_captures_start - 1;
            }
            else
            {
                stage = none;
                return {null_move, 0};
            }
            [[fallthrough]];
        case quiet_moves:
            current++;
            if (*current == pv) current++;
            if (current >= moves.last)
            {
                stage = bad_capture_moves;
                current = &moves.list[255] + 1;
            }
            else
            {
                return {*current, scores[current - moves.begin()]};
            }
            [[fallthrough]];
        case bad_capture_moves:
            current--;
            if (*current == pv) current--;
            if (current <= bad_captures_end)
            {
                stage = none;
            }
            else
            {
                return {*current, -1};
            }
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

    void sort_mvv_capthist(Move* begin, Move*& end)
    {
        if (begin == end) return;

        const bool stm = pos->side_to_move;

        const auto captured_history = [&](const long long index, const Move* move)
        {
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
        };

        captured_history(0, begin);

        Move* start = begin + 1;

        while (start < end)
        {
            auto i = start - begin;

            Move* tmp = start;

            captured_history(i, tmp);

            while (i > 0 && scores[i - 1] < scores[i])
            {
                std::swap(scores[i - 1], scores[i]);
                std::swap(*(tmp - 1), *tmp);

                i--;
                tmp--;
            }
            start++;
        }
    }

    void sort_history(Move* begin, const Move* end)
    {
        if (begin == end) return;

        const auto offset = non_captures_start - moves.begin();
        const auto prev = (ss - 1)->piece_to != UINT16_MAX ? (ss - 1)->piece_to : 0;
        const auto prev2 = (ss - 2)->piece_to != UINT16_MAX ? (ss - 2)->piece_to : 0;

        const auto quiet_history_score = [&](const long long index, const Move* move)
        {
            if (Killers::find(*move, ss->plies)) scores[index] = INT16_MAX;
            else
            {
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
        };

        quiet_history_score(offset, begin);

        Move* start = begin + 1;

        while (start < end)
        {
            auto i = offset + start - begin;
            Move* tmp = start;

            quiet_history_score(i, tmp);

            while (i > offset && scores[i - 1] < scores[i])
            {
                std::swap(scores[i - 1], scores[i]);
                std::swap(*(tmp - 1), *tmp);

                i--;
                tmp--;
            }
            start++;
        }
    }
};
