#pragma once

#include "history.hpp"
#include "movegen.hpp"
#include "move.hpp"
#include "see.hpp"

static constexpr int16_t mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
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
    MoveList moves;
    Move pv;
    Move* non_captures_start;
    uint8_t size;
    Stages stage = generating_capture_moves;
    Position* pos;
    Move* current;
    Move bad_captures[32];
    Move* bad_captures_end;

    explicit MovePicker(Position& _pos, const Move& _pv)
    {
        pos = &_pos;
        if (_pv != move_none && _pos.is_pseudo_legal(_pv)) {
            pv = _pv;
            stage = TT_moves;
        }
        bad_captures_end = bad_captures;
    }

    Move pick()
    {
        switch (stage) {
            case TT_moves:
                stage = generating_capture_moves;
                return pv;
            case generating_capture_moves:
                pseudo_legals<captures>(*pos, moves);
                non_captures_start = moves.last;
                sort_mvv_lva<true>(moves.begin(), non_captures_start);
                moves.last = non_captures_start;
                stage = good_capture_moves;
                current = moves.begin();
            case good_capture_moves:
                if (*current == pv) current++;
                if (current >= non_captures_start) stage = generating_quiet_moves;
                else return *(current++);
            case generating_quiet_moves:
                pseudo_legals<quiet>(*pos, moves);
                sort_history(non_captures_start, moves.last);
                stage = quiet_moves;
            case quiet_moves:
                if (*current == pv) current++;
                if (current >= moves.end()) {
                    stage = bad_capture_moves;
                    current = bad_captures;
                    sort_mvv_lva<false>(bad_captures, bad_captures_end);
                }
                else {
                    return *(current++);
                }
            case bad_capture_moves:
                if (*current == pv) current++;
                if (current >= bad_captures_end) {
                    stage = none;
                }
                else return *(current++);
            case none: default:
                return move_none;
        }
    }

    Move next_move()
    {
        Move move = pick();
        if (move == move_none) return move_none;

        while (!check_move_legality(*pos, move)) {
            move = pick();
            if (move == move_none) return move_none;
        }
        return move;
    }
    template<const bool normal>
    void sort_mvv_lva(Move* begin, Move*& end)
    {
        if (begin == end) return;

        std::array<int16_t, 218> scores;

        scores[0] = mvv_lva[pos->piece_on[begin->src()]][pos->piece_on[begin->dest()]];
        if (begin->flag() >= knight_promo_capture) scores[0] += value_of(begin->promoted_to<white>());

        Move* start = begin + 1;

        while (start < end) {
            int i = start - begin;

            if constexpr (normal)
                if (static_exchange_evaluation(*pos, *start) < 0) {
                    *(bad_captures_end++) = *start;
                    *start = *(--end);
                    continue;
                }

            Move* tmpm = start;
            scores[i] = mvv_lva[pos->piece_on[start->src()]][pos->piece_on[start->dest()]];
            if (start->flag() >= knight_promo_capture) scores[0] += value_of(start->promoted_to<white>());

            while (i > 0 && scores[i - 1] < scores[i]) {
                const int16_t tmp = scores[i - 1];
                scores[i - 1] = scores[i];
                scores[i] = tmp;

                const Move t = *(tmpm - 1);
                *(tmpm - 1) = *tmpm;
                *tmpm = t;

                i--;
                tmpm--;
            }
            start++;
        }
    }

    void sort_history(Move* begin, const Move* end) const
    {
        if (begin == end) return;

        std::array<int16_t, 218> scores;

        scores[0] = History::table[pos->side_to_move][begin->src()][begin->dest()];

        Move* start = begin + 1;

        while (start < end) {
            int i = start - begin;
            Move* tmpm = start;
            scores[i] = History::table[pos->side_to_move][tmpm->src()][tmpm->dest()];

            while (i > 0 && scores[i - 1] < scores[i]) {
                const int16_t tmp = scores[i - 1];
                scores[i - 1] = scores[i];
                scores[i] = tmp;

                const Move t = *(tmpm - 1);
                *(tmpm - 1) = *tmpm;
                *tmpm = t;

                i--;
                tmpm--;
            }
            start++;
        }
    }
};
