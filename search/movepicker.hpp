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
    SearchEntry* ss;
    Move* current;
    Move* bad_captures_end;
    bool capture_only;
    int16_t scores[256];
    int threshold;

    explicit MovePicker(Position* _pos, const bool _capture_only, const Move _pv, SearchEntry* _ss, int _threshold = 0): pos(_pos), ss(_ss), capture_only(_capture_only)
    {
        bad_captures_end = &moves.list[255];
        threshold = _threshold;
        if (_pv && pos->is_pseudo_legal(_pv) && !(capture_only && _pv.flag() != capture && _pv.flag() < knight_promo_capture && _pv.flag() != ep_capture)) {
            if (!pos->state->checker
                || pos->state->check_blocker & 1ull << _pv.dest()
                || pos->piece_on[_pv.src()] == K || pos->piece_on[_pv.src()] == k) {
                pv = _pv;
                stage = TT_moves;
            }
        }
    }

    std::pair<Move, int16_t> pick()
    {
        switch (stage) {
            case TT_moves:
                stage = generating_capture_moves;
                return {pv, 0};
            case generating_capture_moves:
                stage = good_capture_moves;
                pseudo_legals<captures>(*pos, moves);
                sort_mvv_capthist(moves.begin(), moves.last);
                non_captures_start = moves.last;
                current = moves.begin() - 1;
                [[fallthrough]];
            case good_capture_moves:
                current++;
                if (*current == pv) current++;
                if (current >= non_captures_start) {
                    stage = generating_quiet_moves;
                }
                else {
                    while (current < non_captures_start && (*current == pv || static_exchange_evaluation(*pos, *current) < threshold)) {
                        if (*current != pv) {
                            *bad_captures_end-- = *current;
                        }
                        current++;
                    }

                    if (current < non_captures_start) {
                        return {*current, scores[current - moves.begin()]};
                    }
                    stage = generating_quiet_moves;
                }
                [[fallthrough]];
            case generating_quiet_moves:
                if (!capture_only) {
                    pseudo_legals<quiet>(*pos, moves);
                    sort_history(non_captures_start, moves.last);
                    stage = quiet_moves;
                    current = non_captures_start - 1;
                }
                else {
                    stage = none;
                    return {move_none, 0};
                }
                [[fallthrough]];
            case quiet_moves:
                current++;
                if (*current == pv) current++;
                if (current >= moves.last) {
                    stage = bad_capture_moves;
                    current = &moves.list[255] + 1;
                }
                else {
                    return {*current, scores[current - moves.begin()]};
                }
                [[fallthrough]];
            case bad_capture_moves:
                current--;
                if (*current == pv) current--;
                if (current <= bad_captures_end) {
                    stage = none;
                }
                else {
                    return {*current, -1};
                }
                [[fallthrough]];
            case none: default:
                return {move_none, 0};
        }
    }

    std::pair<Move, int16_t> next_move()
    {
        auto move = pick();
        if (!move.first) return {move_none, 0};
        while (!check_move_legality(*pos, move.first)) {
            move = pick();
            if (!move.first) return {move_none, 0};
        }
        return move;
    }

    void sort_mvv_capthist(Move* begin, Move*& end)
    {
        if (begin == end) return;

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

    void sort_history(Move* begin, const Move* end)
    {
        if (begin == end) return;

        uint8_t moved;
        const uint8_t offset = non_captures_start - moves.begin();

        const int16_t prev = ss->piece_to != UINT16_MAX ? ss->piece_to : 0;
        const int16_t prev2 = (ss - 1)->piece_to != UINT16_MAX ? (ss - 1)->piece_to : 0;

        if (Killers::find(*begin, pos->state->ply_from_root)) scores[offset] = INT16_MAX;
        else {
            moved = pos->piece_on[begin->src()];
            if (moved >= 6) moved -= 6;
            scores[offset] = History::table[pos->side_to_move][begin->src()][begin->dest()] +
            2 * Continuation::counter_moves[pos->side_to_move][prev >> 6][prev & 0b111111][moved][begin->dest()]
            + Continuation::follow_up[pos->side_to_move][prev2 >> 6][prev2 & 0b111111][moved][begin->dest()];
        }

        Move* start = begin + 1;

        while (start < end) {
            int i = offset + start - begin;
            Move* tmpm = start;

            if (Killers::find(*tmpm, pos->state->ply_from_root)) scores[i] = INT16_MAX;
            else {
                moved = pos->piece_on[tmpm->src()];
                if (moved >= 6) moved -= 6;
                scores[i] = History::table[pos->side_to_move][tmpm->src()][tmpm->dest()] +
                2 * Continuation::counter_moves[pos->side_to_move][prev >> 6][prev & 0b111111][moved][tmpm->dest()]
                + Continuation::follow_up[pos->side_to_move][prev2 >> 6][prev2 & 0b111111][moved][tmpm->dest()];
            }
            while (i > offset && scores[i - 1] < scores[i]) {
                std::swap(scores[i-1], scores[i]);
                std::swap(*(tmpm - 1), *tmpm);

                i--;
                tmpm--;
            }
            start++;
        }
    }
};