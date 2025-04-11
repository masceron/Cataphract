#pragma once

#include <cstdint>
#include <list>
#include <cmath>
#include <iomanip>

#include "movegen.hpp"
#include "movepicker.hpp"
#include "move.hpp"
#include "see.hpp"
#include "timer.hpp"
#include "../eval/eval.hpp"
#include "../eval/transposition.hpp"

inline static uint64_t node_searched = 0;
inline static uint16_t seldepth;

inline std::list<Move> principal_variation;

inline int16_t quiesce(Position& pos, int16_t alpha, const int16_t beta, std::list<Move>& pv)
{
    node_searched++;
    seldepth = pos.state->ply_from_root;
    if (is_search_cancelled) return 0;

    const int stand_pat = eval(pos);

    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    static constexpr int16_t swing = queen_weight + 200;

    if (stand_pat + swing < alpha) return alpha;

    int best_score = stand_pat;

    MoveList capture_moves;

    legals<captures>(pos, capture_moves);

    State st;

    for (int i = 0; i < capture_moves.size(); i++) {

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

inline uint8_t current_depth;

inline int16_t search(Position& pos, int16_t alpha, int16_t beta, const int depth, std::list<Move>& pv)
{
    node_searched++;

    if (is_search_cancelled) return 0;

    Move depth_best_move = move_none;

    seldepth = pos.state->ply_from_root;

    bool ok = true;
    const int16_t o_alpha = alpha;
    const auto entry = Table::probe(pos.state->key, ok);

    Move tt_move = move_none;

    if (ok) {
        if (entry->depth >= current_depth && pos.state->rule_50 < 40) {
            auto score = entry->score;
            switch (entry->type) {
                case pv_node:
                    if (score < -mate_bound) score += pos.state->ply_from_root;
                    else if (score > mate_bound) score -= pos.state->ply_from_root;
                return score;
                case cut_node:
                    beta = std::min(beta, score);
                break;
                case all_node:
                    alpha = std::max(alpha, score);
                break;
            }
            if (alpha >= beta) return score;
        }
        if (entry->best_move != move_none)
            tt_move = entry->best_move;
    }

    MovePicker move_picker(pos, tt_move);
    Move picked_move = move_picker.next_move();

    if (pos.state->repetition == 3 || (picked_move == move_none && !pos.state->checker) || pos.state->rule_50 >= 50) {
        return draw;
    }

    if (picked_move == move_none) {
        return -mate_value + pos.state->ply_from_root;
    }

    if (depth == 0) return quiesce(pos, alpha, beta, pv);

    std::list<Move> tmp;
    State st;

    pos.make_move(picked_move, st);
    int16_t max = -search(pos, -beta, -alpha, depth - 1, tmp);
    pos.unmake_move(picked_move);

    if (is_search_cancelled) return 0;

    if (max > alpha) {
        if (max >= beta) {
            Table::write(entry, pos.state->key, depth_best_move, pos.state->ply_from_root, max, cut_node);
            return beta;
        }
        alpha = max;

        pv.clear();
        pv.push_back(picked_move);
        pv.splice(pv.end(), tmp);
    }

    picked_move = move_picker.next_move();
    while (picked_move != move_none) {

        std::list<Move> local_pv;

        pos.make_move(picked_move, st);
        int16_t score = -search(pos, -alpha - 1, -alpha, depth - 1, local_pv);
        if (score > alpha && score < beta) {
            score = -search(pos, -beta, -alpha, depth - 1, local_pv);
            if (score > alpha) alpha = score;
        }

        pos.unmake_move(picked_move);

        if (is_search_cancelled) return 0;

        if (score > max) {
            if (score >= beta) {
                max = beta;
                break;
            }

            max = score;
            depth_best_move = picked_move;

            pv.clear();
            pv.push_back(picked_move);
            pv.splice(pv.end(), local_pv);
        }
        picked_move = move_picker.next_move();
    }

    if (max != 0) {
        NodeType type;
        if (max <= o_alpha) {
            type = cut_node;
        }
        else if (max >= beta) {
            type = all_node;
        }
        else {
            type = pv_node;
        }
        Table::write(entry, pos.state->key, depth_best_move, pos.state->ply_from_root, max, type);
    }

    return max;
}


inline void start_search(const int depth)
{
    Timer::start();

    node_searched = 0;

    const bool us = position.side_to_move;

    MoveList moves;
    legals<all>(position, moves);
    Move best_move = moves.list[0];

    for (int cr_depth = 1; cr_depth <= depth; cr_depth++) {
        if (is_search_cancelled) break;

        int16_t max_score = negative_infinity;
        current_depth = cr_depth;

        for (int i = 0; i < moves.size(); i++) {
            State st;
            std::list<Move> local_pv;
            Move picked_move = moves.list[i];

            position.make_move(picked_move, st);
            const int16_t score = -search(position, negative_infinity, infinity, cr_depth - 1, local_pv);
            position.unmake_move(picked_move);

            if (is_search_cancelled) break;

            if (score > max_score) {
                best_move = picked_move;
                max_score = score;

                principal_variation.clear();
                principal_variation.push_back(picked_move);
                principal_variation.splice(principal_variation.end(), local_pv);

                const Move tmp = moves.list[0];
                moves.list[0] = picked_move;
                moves.list[i] = tmp;
            }

        }

        const double elapsed = Timer::elapsed();

        std::cout << "info depth " << cr_depth << " seldepth " << seldepth << " score";

        if (max_score < -mate_bound) std::cout << " mate " << - std::ceil((mate_value + max_score) / 2.0);
        else if (max_score > mate_bound) std::cout << " mate " << std::ceil((mate_value - max_score) / 2.0);
        else std::cout << " cp " << max_score * (us == white ? 1 : -1);

        std::cout << " nodes " << node_searched
        << " nps " << std::fixed << std::setprecision(0) << node_searched / elapsed * 1000000  << " pv ";

        for (auto x : principal_variation) {
            std::cout << get_move_string(x) << " ";
        }

        std::cout << "\n";
    }

    Timer::stop();
    Timer::timer_thread.join();

    std::cout << "bestmove " << get_move_string(best_move) << "\n";
}