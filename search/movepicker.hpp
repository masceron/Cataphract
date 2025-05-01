#pragma once

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
    MoveList moves;
    Move pv = move_none;
    Move* non_captures_start;
    Stages stage = generating_capture_moves;
    Position* pos;
    Move* current;
    Move bad_captures[32];
    Move* bad_captures_end;
    bool capture_only;

    explicit MovePicker(Position* _pos, const bool _capture_only, const Move& _pv): pos(_pos), capture_only(_capture_only)
    {
        bad_captures_end = bad_captures;
        if (_pv != move_none && pos->is_pseudo_legal(_pv) && !(capture_only && _pv.flag() != capture && _pv.flag() < knight_promo_capture)) {
            if (!pos->state->checker || pos->state->check_blocker & 1ull << _pv.dest()) {
                pv = _pv;
                stage = TT_moves;
            }
        }
    }

    Move pick()
    {
        switch (stage) {
            case TT_moves:
                stage = generating_capture_moves;
                return pv;
            case generating_capture_moves:
                stage = good_capture_moves;
                pseudo_legals<captures>(*pos, moves);
                sort_mvv_capthist(moves.begin(), moves.last);
                non_captures_start = moves.last;
                current = moves.begin();
            case good_capture_moves:
                if (*current == pv) current++;
                if (current >= non_captures_start) {
                    stage = generating_quiet_moves;
                }
                else {
                    Move picked;
                    int16_t last_sse = -1;
                    while (current < non_captures_start && ((last_sse = static_exchange_evaluation(*pos, picked = *current++) < 0))) {
                        if (*current != pv)
                            *bad_captures_end++ = picked;
                        else current++;
                    }
                    if (last_sse >= 0)
                        return picked;
                    stage = generating_quiet_moves;
                }
            case generating_quiet_moves:
                if (!capture_only) {
                    pseudo_legals<quiet>(*pos, moves);
                    sort_history(non_captures_start, moves.last);
                    stage = quiet_moves;
                    current = non_captures_start;
                }
                else {
                    stage = none;
                    return move_none;
                }
            case quiet_moves:
                if (*current == pv) current++;
                if (current >= moves.last) {
                    stage = bad_capture_moves;
                    current = bad_captures;
                }
                else {
                    return *(current++);
                }
            case bad_capture_moves:
                if (*current == pv) current++;
                if (current >= bad_captures_end) {
                    stage = none;
                }
                else {
                    return *(current++);
                }
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

    void sort_mvv_capthist(Move* begin, Move*& end) const
    {
        if (begin == end) return;

        std::array<int16_t, 218> scores;

        const bool stm = pos->side_to_move;
        uint8_t moved = pos->piece_on[begin->src()];
        uint8_t captured = pos->piece_on[begin->dest()];

        if (captured == nil) captured = P;
        if (moved >= 6) moved -= 6;
        if (captured >= 6) captured -= 6;

        scores[0] = mvv[captured] + Capture::table[stm][moved][captured][begin->dest()];
        if (begin->flag() >= knight_promo_capture) scores[0] += value_of(begin->promoted_to<false>());

        Move* start = begin + 1;

        while (start < end) {
            int i = start - begin;

            Move* tmpm = start;

            moved = pos->piece_on[start->src()];
            captured = pos->piece_on[start->dest()];

            if (captured == nil) captured = P;
            if (moved >= 6) moved -= 6;
            if (captured >= 6) captured -= 6;

            scores[i] = mvv[captured] + Capture::table[stm][moved][captured][start->dest()];
            if (start->flag() >= knight_promo_capture) scores[i] += value_of(start->promoted_to<false>());

            while (i > 0 && scores[i - 1] < scores[i]) {
                std::swap(scores[i-1], scores[i]);
                std::swap(*(tmpm - 1), *tmpm);

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
        uint8_t moved;

        const auto prev = !Continuation::search_stack.empty() ? Continuation::search_stack.back() : 0;
        const auto prev2 = Continuation::search_stack.size() >= 2 ? Continuation::search_stack[Continuation::search_stack.size() - 2] : 0;

        if (Killers::find(*begin, pos->state->ply_from_root)) scores[0] = INT16_MAX;
        else {
            moved = pos->piece_on[begin->src()];
            if (moved >= 6) moved -= 6;
            scores[0] = History::table[pos->side_to_move][begin->src()][begin->dest()] +
            2 * Continuation::counter_moves[pos->side_to_move][prev >> 6][prev & 0b111111][moved][begin->dest()]
            + Continuation::follow_up[pos->side_to_move][prev2 >> 6][prev2 & 0b111111][moved][begin->dest()];
        }

        Move* start = begin + 1;

        while (start < end) {
            int i = start - begin;
            Move* tmpm = start;

            if (Killers::find(*tmpm, pos->state->ply_from_root)) scores[i] = INT16_MAX;
            else {
                moved = pos->piece_on[tmpm->src()];
                if (moved >= 6) moved -= 6;
                scores[i] = History::table[pos->side_to_move][tmpm->src()][tmpm->dest()] +
                2 * Continuation::counter_moves[pos->side_to_move][prev >> 6][prev & 0b111111][moved][tmpm->dest()]
                + Continuation::follow_up[pos->side_to_move][prev2 >> 6][prev2 & 0b111111][moved][tmpm->dest()];
            }
            while (i > 0 && scores[i - 1] < scores[i]) {
                std::swap(scores[i-1], scores[i]);
                std::swap(*(tmpm - 1), *tmpm);

                i--;
                tmpm--;
            }
            start++;
        }
    }
};