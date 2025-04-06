#pragma once

#include <cstdint>
#include <array>

#include "movegen.hpp"
#include "timer.hpp"
#include "../eval/eval.hpp"

enum Values
{
    negative_infinity = -INT32_MAX,
    infinity = INT32_MAX
};

inline static uint64_t node_searched = 0;

inline int32_t quiesce(Position& pos, int alpha, const int beta)
{
    if (is_search_cancelled) return 0;

    const int stand_pat = eval(pos);
    node_searched++;

    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    static constexpr int32_t swing = queen_weight + 200;

    if (stand_pat + swing < alpha) return alpha;

    int best_score = stand_pat;

    const MoveList capture_list = capture_move_generator(pos);

    State st;
    for (int i = 0; i < capture_list.size(); i++) {

        pos.make_move(capture_list.list[i], st);
        const int32_t score = -quiesce(pos, -beta, -alpha);
        pos.unmake_move(capture_list.list[i]);

        if (score >= beta) return score;
        if (score >= best_score) best_score = score;
        if (score > alpha) alpha = score;
    }

    return best_score;
}

inline int search(Position& pos, int alpha, const int beta, const int depth)
{
    if (pos.state->repetition == 3 || pos.state->rule_50 >= 50) {
        node_searched++;
        return 0;
    }

    if (depth == 0) return quiesce(pos, alpha, beta);

    int max = negative_infinity;

    const MoveList list = legal_move_generator(pos);
    for (int i = 0; i < list.size(); i++) {

        if (is_search_cancelled) return 0;

        State st;
        pos.make_move(list.list[i], st);
        const int score = -search(pos, -beta, -alpha, depth - 1);
        pos.unmake_move(list.list[i]);
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

    const MoveList list = legal_move_generator(position);

    std::array<int32_t, 218> scores;
    std::ranges::fill(scores, negative_infinity);

    node_searched = 0;

    const bool us = position.side_to_move;
    State st;
    for (int cr_depth = 1; cr_depth <= depth; cr_depth++) {
        if (is_search_cancelled) break;

        int iteration_max = 0;

        for (int i = 0; i < list.size(); i++) {

            position.make_move(list.list[i], st);
            scores[i] = -search(position, negative_infinity, infinity, cr_depth - 1);
            position.unmake_move(list.list[i]);

            if (scores[i] > scores[iteration_max]) iteration_max = i;

            if (is_search_cancelled) break;
        }

        const auto temp = scores[iteration_max];
        scores[iteration_max] = scores[0];
        scores[0] = temp;

        std::cout << "Current best move at depth " << cr_depth << ": " << get_move_string(list.list[0])
        <<" (" << scores[0] * (us == black ? - 1 : 1) << ")"
        << ". Node searched: " << node_searched << ".\n";
    }

    Timer::stop();
    Timer::timer_thread.join();

    return get_move_string(list.list[0]);
}