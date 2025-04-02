#pragma once

#include <cstdint>

#include "movegen.hpp"
#include "timer.hpp"
#include "../eval/eval.hpp"

inline int quiesce(int alpha, const int beta)
{
    const int stand_pat = eval();
    int best_score = stand_pat;
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;
    const MoveList capture_list = capture_move_generator();
    for (int i = 0; i < capture_list.size(); i++) {
        State st;
        position.make_move(capture_list.list[i], st);
        const int score = -quiesce(-beta, -alpha);
        position.unmake_move(capture_list.list[i]);

        if (score >= beta) return score;
        if (score >= best_score) best_score = score;
        if (score > alpha) alpha = score;
    }

    return best_score;
}

inline int search(int alpha, const int beta, const int depth)
{
    if (position.state->repetition == 3) return 0;

    if (depth == 0) return quiesce(alpha, beta);

    int max = INT32_MIN;

    const MoveList list = legal_move_generator();
    for (int i = 0; i < list.size(); i++) {

        if (is_search_cancelled) return 0;

        State st;
        position.make_move(list.list[i], st);
        const int score = -search(-beta, -alpha, depth - 1);
        position.unmake_move(list.list[i]);
        if (score > max) {
            max = score;
            if (score > alpha) {
                alpha = score;
            }
        }
        if (score >= beta) {
            return max;
        }
    }
    return max;
}


inline std::string start_search(const int depth)
{
    Timer::start();

    const MoveList list = legal_move_generator();
    int scores[218];
    int total_max = 0;
    for (int cr_depth = 1; cr_depth <= depth; cr_depth++) {
        if (is_search_cancelled) break;

        int iteration_max = 0;
        for (int i = 0; i < list.size(); i++) {
            State st;

            position.make_move(list.list[i], st);
            scores[i] = search(INT32_MIN, INT32_MAX, cr_depth);
            position.unmake_move(list.list[i]);

            if (scores[i] > scores[iteration_max]) iteration_max = i;

            if (is_search_cancelled) break;
        }
        if (scores[iteration_max] > scores[total_max]) total_max = iteration_max;

        std::cout << "Current best move at depth " << cr_depth << ": " << get_move_string(list.list[total_max]) << "\n";
    }

    Timer::stop();
    Timer::timer_thread.join();

    return get_move_string(list.list[total_max]);
}
