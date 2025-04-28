#pragma once

#include <cmath>
#include <forward_list>
#include <list>
#include <iomanip>

#include "history.hpp"
#include "../position/movegen.hpp"
#include "movepicker.hpp"
#include "../position/move.hpp"
#include "see.hpp"
#include "timer.hpp"
#include "../eval/eval.hpp"
#include "../eval/transposition.hpp"

inline std::array<std::array<uint8_t, 63>, 127> reductions_cals()
{
    std::array<std::array<uint8_t, 63>, 127> r;
    for (int depth = 0; depth < 127; depth++) {
        for (int numMoves = 0 ; numMoves < 63; numMoves++) {
            r[depth][numMoves] = std::floor(0.5 + std::log(depth + 1) * std::log(numMoves + 1) / 3.0);
        }
    }
    return r;
}

inline std::array<std::array<uint8_t, 63>, 127> reductions = reductions_cals();

inline static uint64_t node_searched = 0;
inline static uint16_t seldepth;

inline static constexpr uint8_t max_ply = 32;

inline int16_t quiesce(Position& pos, int16_t alpha, const int16_t beta)
{
    node_searched++;
    if (pos.state->ply_from_root > seldepth) {
        seldepth = pos.state->ply_from_root;
    }

    if (is_search_cancelled) return 0;

    int16_t saved_eval;
    bool ok = false;
    const auto entry = TT::probe(pos.state->key, ok, 0, pos.state->ply_from_root, alpha, beta, saved_eval);

    int stand_pat = eval(pos);
    if (ok && !((stand_pat > saved_eval && entry->type == lower_bound)
        || (stand_pat < saved_eval && entry->type == upper_bound)))
        stand_pat = saved_eval;

    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    int best_score = stand_pat;

    MovePicker move_picker(&pos, true);
    Move picked_move;

    State st;

    NodeType type = upper_bound;
    Move best_move = move_none;

    while ((picked_move = move_picker.next_move()) != move_none) {
        //Delta pruning: https://www.chessprogramming.org/Delta_Pruning
        if (picked_move.flag() < knight_promotion && stand_pat + value_of(pos.piece_on[picked_move.dest()]) + 200 < alpha) {
            continue;
        }

        accumulator_stack.emplace_back(pos, picked_move);
        pos.make_move(picked_move, st);
        const int16_t score = -quiesce(pos, -beta, -alpha);
        accumulator_stack.pop_back();
        pos.unmake_move(picked_move);

        if (is_search_cancelled) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = picked_move;
            if (score > alpha) {
                if (score >= beta) {
                    type = lower_bound;
                    break;
                }
                alpha = score;
            }
        }
    }

    TT::write(entry, pos.state->key, best_move, 0, pos.state->ply_from_root, best_score, type);

    return best_score;
}

inline Move best_move;

inline int16_t search(Position& pos, int16_t alpha, int16_t beta, const int8_t depth, std::list<Move>& pv, const bool allow_null)
{
    node_searched++;

    if (is_search_cancelled) return 0;

    Move depth_best_move = move_none;

    if (pos.state->ply_from_root > seldepth) {
        seldepth = pos.state->ply_from_root;
    }

    //If depth <= 0 call quiescence search: https://www.chessprogramming.org/Quiescence_Search
    if (depth <= 0) {
        return quiesce(pos, alpha, beta);
    }

    if (pos.state->ply_from_root > max_ply) {
        return eval(pos);
    }

    //Mate Distance Pruning: https://www.chessprogramming.org/Mate_Distance_Pruning
    if (pos.state->ply_from_root) {
        alpha = std::max(alpha, static_cast<int16_t>(-mate_value + pos.state->ply_from_root));
        beta = std::min(beta, static_cast<int16_t>(mate_value - pos.state->ply_from_root - 1));
        if (alpha >= beta) return alpha;
    }

    MovePicker move_picker(&pos, false);
    Move picked_move = move_picker.next_move();

    const bool not_in_check = !pos.state->checker;

    //Checking for draw.
    if (pos.state->repetition >= 3 || (picked_move == move_none && not_in_check) || pos.state->rule_50 >= 100) {
        return draw;
    }
    //Checking for mate.
    if (picked_move == move_none) {
        return -mate_value + pos.state->ply_from_root;
    }

    bool ok = false;

    const bool is_pv = beta - alpha > 1;

    int16_t saved_eval;
    // Probe the transposition table: https://www.chessprogramming.org/Transposition_Table
    const auto entry = TT::probe(pos.state->key, ok, depth, pos.state->ply_from_root, alpha, beta, saved_eval);
    if (ok) {
        if (entry->type == exact || (entry->type == lower_bound && saved_eval >= beta) || (entry->type == upper_bound && saved_eval <= alpha))
            return saved_eval;
    }

    int16_t static_eval = eval(pos);
    if (ok && !((static_eval > saved_eval && entry->type == lower_bound) || (static_eval < saved_eval && entry->type == upper_bound)))
        static_eval = saved_eval;

    move_picker.set_pv(pos.state->ply_from_root ? entry->best_move : best_move);

    bool futility_pruning_allowed = false;

    if (pos.state->ply_from_root && !is_pv && not_in_check) {
        //Reverse futility pruning: https://www.chessprogramming.org/Reverse_Futility_Pruning
        if (depth <= 9) {
            const int16_t margin = 120 * depth;
            if (static_eval < mate_bound && static_eval - margin >= beta)
                return beta + (static_eval - beta) / 3;
        }

        //Razoring: https://www.chessprogramming.org/Razoring
        if (depth <= 5) {
            if (static_eval + 230 * depth <= alpha) {
                if (quiesce(pos, alpha, beta) < alpha) return alpha;
            }
        }

        //Null move pruning: https://www.chessprogramming.org/Null_Move_Pruning
        if (allow_null && depth >= 3 &&
        std::popcount(pos.occupations[2]) - std::popcount(pos.boards[P]) - std::popcount(pos.boards[p]) > 2) {
            if (static_eval > beta) {
                const uint8_t r = std::min((static_eval - beta) / 232, 6) + depth / 3 + 5;
                State st;
                std::list<Move> local_pv;
                pos.make_null_move(st);
                const int16_t null_score = -search(pos, -beta, -beta + 1, depth - r, local_pv, false);
                pos.unmake_null_move();

                if (is_search_cancelled) return 0;
                if (null_score >= beta) return beta;
            }
        }

        if (depth <= 9) {
            if (static_eval == negative_infinity) static_eval = eval(pos);
            if (static_eval + 140 * depth <= alpha) futility_pruning_allowed = true;
        }
    }

    std::forward_list<Move> quiets_searched;
    std::forward_list<CaptureEntry> capture_searched;

    //Check extension: https://www.chessprogramming.org/Check_Extensions
    const int8_t extension = not_in_check ? 0 : 1;

    uint8_t move_searched = 0;
    int16_t best_score = negative_infinity;
    NodeType type = upper_bound;

    while (picked_move != move_none) {
        if (futility_pruning_allowed) {
            if (move_picker.stage == quiet_moves) {
                picked_move = move_picker.next_move();
                continue;
            }
        }

        std::list<Move> local_pv;

        accumulator_stack.emplace_back(pos, picked_move);

        int16_t score;
        State st;
        pos.make_move(picked_move, st);
        //Principal variation search: https://www.chessprogramming.org/Principal_Variation_Search
        if (move_searched == 0) {
            score = -search(pos, -beta, -alpha, depth - 1 + extension, local_pv, true);
        }
        else {
            //Late move reductions: https://www.chessprogramming.org/Late_Move_Reductions
            const int8_t reduction = (picked_move.flag() < knight_promotion) ? reductions[depth - 1][move_searched - 1] : 0;
            score = -search(pos, -alpha - 1, -alpha, depth - 1 + extension - reduction, local_pv, true);
            if (score > alpha && beta - alpha > 1) {
                score = -search(pos, -beta, -alpha, depth - 1 + extension, local_pv, true);
            }
        }
        accumulator_stack.pop_back();
        pos.unmake_move(picked_move);

        if (is_search_cancelled) return 0;
        move_searched++;

        if (score > best_score) {
            best_score = score;
            depth_best_move = picked_move;
            if (score > alpha) {
                type = exact;
                if (score >= beta) {
                    if (picked_move.flag() == capture || picked_move.flag() >= knight_promo_capture) {
                        Pieces captured = pos.piece_on[picked_move.dest()];
                        if (captured == nil) captured = pos.side_to_move ? P : p;
                        Capture::update(capture_searched, pos.piece_on[picked_move.src()], captured, picked_move.dest(), depth);
                    }
                    else {
                        //History heuristic: https://www.chessprogramming.org/History_Heuristic
                        Killers::insert(picked_move, pos.state->ply_from_root);
                        History::update(quiets_searched, pos.side_to_move, picked_move.src(), picked_move.dest(), depth);
                    }
                    type = lower_bound;
                    break;
                }

                alpha = score;
                pv.clear();
                pv.push_back(picked_move);
                pv.splice(pv.end(), local_pv);
            }
        }

        if (picked_move.flag() == capture || picked_move.flag() >= knight_promo_capture) {
            Pieces captured = pos.piece_on[picked_move.dest()];
            if (captured == nil) captured = pos.side_to_move ? P : p;
            capture_searched.emplace_front(pos.piece_on[picked_move.src()], captured, picked_move.dest());
        }
        else {
            quiets_searched.push_front(picked_move);
        }

        picked_move = move_picker.next_move();
    }

    TT::write(entry, pos.state->key, depth_best_move, depth, pos.state->ply_from_root, best_score, type);

    if (const int16_t delta = best_score - static_eval;
        not_in_check &&
        ((depth_best_move.flag() != capture && depth_best_move.flag() < knight_promo_capture) || depth_best_move == move_none)
        && !(type == lower_bound && delta < 0)
        && !(type == upper_bound && delta > 0)) {
        Corrections::update(delta, pos, depth);
    }

    return best_score;
}


inline void start_search(int depth, const int time)
{
    if (time != - 1) {
        depth = 128;
        Timer::start(time - 500);
    }
    else Timer::start(60000);

    node_searched = 0;
    best_move = move_none;
    std::list<Move> principal_variation;

    accumulator_stack.clear();
    accumulator_stack.emplace_back(root_accumulators);

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
        std::cout << std::endl;

        best_move = principal_variation.front();
    }

    Timer::stop();
    Timer::timer_thread.join();

    if (best_move == move_none) best_move = legals<all>(position).list[0];

    std::cout << "bestmove " << get_move_string(best_move) << std::endl;
}