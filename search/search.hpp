#pragma once

#include <cmath>
#include <forward_list>
#include <list>
#include <iomanip>

#include "history.hpp"
#include "movegen.hpp"
#include "movepicker.hpp"
#include "move.hpp"
#include "see.hpp"
#include "timer.hpp"
#include "../eval/eval.hpp"
#include "../eval/transposition.hpp"

inline static uint64_t node_searched = 0;
inline static uint16_t seldepth;

inline static constexpr uint8_t max_ply = 32;

inline int16_t quiesce(Position& pos, int16_t alpha, const int16_t beta, std::list<Move>& pv)
{
    node_searched++;
    seldepth = pos.state->ply_from_root;
    if (is_search_cancelled) return 0;

    const int stand_pat = eval(pos);

    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    static constexpr int16_t swing = queen_weight + 200;

    //Delta pruning: https://www.chessprogramming.org/Delta_Pruning
    if (stand_pat + swing < alpha) return alpha;

    int best_score = stand_pat;

    const MoveList capture_moves = legals<captures>(pos);

    State st;

    for (int i = 0; i < capture_moves.size(); i++) {
        //Do not consider moves in which the exchange is not at least even: https://www.chessprogramming.org/Static_Exchange_Evaluation
        if (static_exchange_evaluation(pos, capture_moves.list[i]) < 0) continue;

        std::list<Move> local_pv;

        pos.make_move(capture_moves.list[i], st);
        const int16_t score = -quiesce(pos, -beta, -alpha, local_pv);
        pos.unmake_move(capture_moves.list[i]);

        if (is_search_cancelled) return 0;

        if (score >= beta) {
            return beta;
        }
        if (score >= best_score) best_score = score;
        if (score > alpha) {
            alpha = score;
            pv.clear();
            pv.push_back(capture_moves.list[i]);
            pv.splice(pv.end(), local_pv);
        }
    }

    return best_score;
}


inline int16_t search(Position& pos, int16_t alpha, int16_t beta, const int8_t depth, std::list<Move>& pv, const bool allow_null)
{
    node_searched++;

    if (is_search_cancelled) return 0;

    Move depth_best_move = move_none;
    seldepth = pos.state->ply_from_root;

    //Mate Distance Pruning: https://www.chessprogramming.org/Mate_Distance_Pruning
    alpha = std::max(alpha, static_cast<int16_t>(-mate_value + pos.state->ply_from_root));
    beta = std::min(beta, static_cast<int16_t>(mate_value - pos.state->ply_from_root));
    if (alpha >= beta) return alpha;

    //If depth <= 0 call quiescence search: https://www.chessprogramming.org/Quiescence_Search
    if (depth <= 0) {
        return quiesce(pos, alpha, beta, pv);
    }

    if (pos.state->ply_from_root > max_ply) {
        return eval(pos);
    }

    MovePicker move_picker(pos);
    Move picked_move = move_picker.next_move();

    //Checking for draw.
    if (pos.state->repetition >= 3 || (picked_move == move_none && !pos.state->checker) || pos.state->rule_50 >= 100) {
        return draw;
    }
    //Checking for mate.
    if (picked_move == move_none) {
        return -mate_value + pos.state->ply_from_root;
    }

    bool ok = false;
    int16_t score;
    const int16_t old_alpha = alpha;

    // Probe the transposition table: https://www.chessprogramming.org/Transposition_Table
    const auto [bucket, entry] = TT::probe(pos.state->key, ok, depth, pos.state->ply_from_root, alpha, beta, score);

    //If the score is usable, i.e. from an identical position with enough depth, return immediately.
    if (ok) return score;

    //Even when the depth of the entry is not sufficient to return the score, we can still recover the best move.
    if (entry != nullptr) {
        move_picker.set_pv(entry->best_move);
    }

    //Set up conditions for futility pruning: https://www.chessprogramming.org/Futility_Pruning
    bool futility_pruning_allowed = false;
    if (!pos.state->checker && depth > 0) {
        static constexpr int16_t futility_margin_base = knight_weight + pawn_weight * 0.5;
        static constexpr int16_t futility_margin_multiplier = pawn_weight * 2;
        if (eval(pos) + futility_margin_base + ((depth - 1) * futility_margin_multiplier) < alpha)
            futility_pruning_allowed = true;
    }

    //Null move pruning: https://www.chessprogramming.org/Null_Move_Pruning
    if (allow_null && !pos.state->checker && depth > 3 && move_picker.pv == move_none
        && (std::popcount(pos.occupations[pos.side_to_move]) - std::popcount(pos.boards[pos.side_to_move == white ? P : p])) > 1) {
        State st;
        const int reduction = 1 + depth / 3;
        pos.make_null_move(st);
        score = -search(pos, -beta, -beta + 1, depth - 1 - reduction, pv, false);
        pos.unmake_null_move();
        if (score >= beta) return beta;
    }

    //A list of quiet moves searched, to apply the penalty to the history table when a quiet move fail high.
    std::forward_list<Move*> quiets_searched;

    //Check extension: https://www.chessprogramming.org/Check_Extensions
    const int8_t extension = pos.state->checker ? 1 : 0;

    //Principal variation search: https://www.chessprogramming.org/Principal_Variation_Search
    std::list<Move> tmp;
    State st;
    pos.make_move(picked_move, st);
    score = -search(pos, -beta, -alpha, depth - 1 + extension, tmp, true);
    pos.unmake_move(picked_move);

    if (is_search_cancelled) return 0;

    if (score > alpha) {
        depth_best_move = picked_move;
        if (score >= beta) {
            if (move_picker.stage == quiet_moves) {
                History::update(quiets_searched, pos.side_to_move, picked_move.src(), picked_move.dest(), pos.state->ply_from_root);
            }
            TT::write(bucket, entry, pos.state->key, depth_best_move, depth, pos.state->ply_from_root, beta, lower_bound);
            return beta;
        }
        alpha = score;

        pv.clear();
        pv.push_back(picked_move);
        pv.splice(pv.end(), tmp);
    }

    if (move_picker.stage == quiet_moves) quiets_searched.push_front(&picked_move);

    picked_move = move_picker.next_move();

    uint8_t move_searched = 0;
    while (picked_move != move_none) {
        if (futility_pruning_allowed) {
            if (move_picker.stage == quiet_moves && !pos.does_move_give_check(picked_move)) {
                picked_move = move_picker.next_move();
                continue;
            }
        }

        std::list<Move> local_pv;

        static constexpr int8_t reduction_limit = 3;
        static constexpr int8_t full_depth = 4;

        //Late move reductions: https://www.chessprogramming.org/Late_Move_Reductions
        const int8_t reduction = (move_searched >= full_depth && depth >= reduction_limit) ? 1 : 0;

        pos.make_move(picked_move, st);
        score = -search(pos, -alpha - 1, -alpha, depth - 1 + extension - reduction, local_pv, true);
        if (score > alpha && beta - alpha > 1) {
            score = -search(pos, -beta, -alpha, depth - 1 + extension, local_pv, true);
        }
        pos.unmake_move(picked_move);
        move_searched++;

        if (is_search_cancelled) return 0;

        if (score > alpha) {
            depth_best_move = picked_move;
            if (score >= beta) {
                if (move_picker.stage == quiet_moves) {
                    //History heuristic: https://www.chessprogramming.org/History_Heuristic
                    History::update(quiets_searched, pos.side_to_move, picked_move.src(), picked_move.dest(), pos.state->ply_from_root);
                }
                TT::write(bucket, entry, pos.state->key, depth_best_move, depth, pos.state->ply_from_root, beta, lower_bound);
                return beta;
            }
            alpha = score;

            pv.clear();
            pv.push_back(picked_move);
            pv.splice(pv.end(), local_pv);
        }

        if (move_picker.stage == quiet_moves) {
            quiets_searched.push_front(&picked_move);
        }

        picked_move = move_picker.next_move();
    }

    if (old_alpha < alpha)
        TT::write(bucket, entry, pos.state->key, depth_best_move, depth, pos.state->ply_from_root, alpha, exact);
    else TT::write(bucket, entry, pos.state->key, depth_best_move, depth, pos.state->ply_from_root, alpha, upper_bound);

    return alpha;
}


inline void start_search(int depth, const int time)
{
    if (time != - 1) {
        depth = 128;
        Timer::start(time - 500);
    }
    else Timer::start(9500);

    node_searched = 0;
    TT::age += age_delta;
    std::list<Move> principal_variation;
    Move best_move = move_none;

    int16_t alpha = negative_infinity;
    int16_t beta = infinity;

    int16_t window = pawn_weight / 2;
    //Iterative deepening: https://www.chessprogramming.org/Iterative_Deepening
    for (int cr_depth = 1; cr_depth <= depth; cr_depth++) {
        if (is_search_cancelled) break;

        int16_t score;

        //Negamax: https://www.chessprogramming.org/Negamax
        while (true) {
            //Aspiration Windows: https://www.chessprogramming.org/Aspiration_Windows
            score = search(position, alpha, beta, cr_depth, principal_variation, true);
            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(static_cast<int16_t>(score - window), static_cast<int16_t>(negative_infinity));
            }
            else if (score >= beta) {
                beta = std::min(static_cast<int16_t>(score + window), static_cast<int16_t>(infinity));
            }
            else {
                break;
            }
            window += window / 2;

            if (is_search_cancelled) break;
        }

        if (is_search_cancelled) break;

        const double elapsed = Timer::elapsed();
        std::cout << "info depth " << cr_depth << " seldepth " << seldepth << " score";

        if (score < -mate_bound) std::cout << " mate " << - std::ceil((mate_value + score) / 2.0);
        else if (score > mate_bound) std::cout << " mate " << std::ceil((mate_value - score) / 2.0);
        else std::cout << " cp " << score;

        std::cout << " nodes " << node_searched
        << " nps " << std::fixed << std::setprecision(0) << node_searched / elapsed * 1000000  << " pv ";

        for (auto x : principal_variation) {
            std::cout << get_move_string(x) << " ";
        }
        std::cout << "\n";

        best_move = principal_variation.front();
    }

    Timer::stop();
    Timer::timer_thread.join();

    if (best_move == move_none) best_move = legals<all>(position).list[0];

    std::cout << "bestmove " << get_move_string(best_move) << "\n";
}