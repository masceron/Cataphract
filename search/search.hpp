#pragma once

#include <cstdint>

#include "movegen.hpp"
#include "../eval/eval.hpp"

inline int quiesce(int alpha, int beta)
{
    const int stand_pat = eval();
    int best_score = stand_pat;
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    return best_score;
}

inline int search(int alpha, int beta, const int16_t depth)
{
    if (depth == 0) return quiesce(alpha, beta);

    constexpr int max = INT32_MIN;

    const MoveList list = legal_move_generator();
    for (int i = 0; i < list.size(); i++) {
        State st;
        position.make_move(list.list[i], st);
        const int score = -search(-beta, -alpha, depth - 1);
        position.unmake_move(list.list[i]);
        if (score > alpha) {
            alpha = score;
        }
        if (score >= beta) {
            return max;
        }
    }
    return max;
}


inline std::string start_search(const int depth)
{
    const MoveList list = legal_move_generator();
    int scores[218];
    int max = 0;
    for (int i = 0; i < list.size(); i++) {
        State st;
        position.make_move(list.list[i], st);
        scores[i] = search(INT32_MIN, INT32_MAX, depth);
        if (scores[i] > scores[max]) max = i;
        position.unmake_move(list.list[i]);
    }

    for (int i = 0; i < list.size(); i++) {
        std::cout << get_move_string(list.list[i]) << ": " << scores[i] << "\n";
    }
    return get_move_string(list.list[max]);
}
