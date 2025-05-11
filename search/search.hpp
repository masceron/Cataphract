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

#ifndef __clang__
constexpr std::array<std::array<uint8_t, 63>,127> reductions_cals()
{
    std::array<std::array<uint8_t, 63>, 127> r;
    for (int depth = 0; depth < 127; depth++) {
        for (int numMoves = 0 ; numMoves < 63; numMoves++) {
            r[depth][numMoves] = std::floor(0.5 + std::log(depth + 1) * std::log(numMoves + 1) / 3.5);
        }
    }
    return r;
}
inline constexpr auto reductions = reductions_cals();
#else
inline std::array<std::array<uint8_t, 63>,127> reductions_cals()
{
    std::array<std::array<uint8_t, 63>, 127> r;
    for (int depth = 0; depth < 127; depth++) {
        for (int numMoves = 0 ; numMoves < 63; numMoves++) {
            r[depth][numMoves] = std::floor(0.5 + std::log(depth + 1) * std::log(numMoves + 1) / 3.3);
        }
    }
    return r;
}
inline auto reductions = reductions_cals();
#endif

inline static uint32_t node_searched = 0;
inline static int seldepth;

inline static constexpr uint8_t max_ply = 127;

inline int quiesce(Position& pos, int alpha, const int beta, SearchEntry* ss)
{
    node_searched++;
    if (pos.state->ply_from_root > seldepth) {
        seldepth = pos.state->ply_from_root;
    }

    if (is_search_cancelled) return alpha;

    if (pos.state->ply_from_root > max_ply) return eval(pos);

    int saved_eval;
    bool match = false;

    const auto entry = TT::probe(pos.state->key, match, pos.state->ply_from_root, saved_eval);
    int raw_static_eval;
    int stand_pat;

    Move best_move = move_none;
    const uint8_t entry_type = entry->age_type & 0b11;
    const bool not_in_check = !pos.state->checker;

    if (match) {
        if (entry_type == exact
                || (entry_type == lower_bound && saved_eval >= beta)
                || (entry_type == upper_bound && saved_eval <= alpha))
                return saved_eval;

        if (not_in_check) {
            if (entry->static_eval != score_none) {
                raw_static_eval = entry->static_eval;
            }
            else {
                raw_static_eval = eval(pos);
            }
            stand_pat = Corrections::correct(raw_static_eval, pos);

            if (!((stand_pat > saved_eval && entry_type == lower_bound) || (stand_pat < saved_eval && entry_type == upper_bound)))
                stand_pat = saved_eval;
        }
        else {
            raw_static_eval = eval(pos);
            stand_pat = Corrections::correct(raw_static_eval, pos);
        }
        best_move = entry->best_move;
    }
    else {
        raw_static_eval = eval(pos);
        stand_pat = Corrections::correct(raw_static_eval, pos);
    }

    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    int best_score = stand_pat;

    MovePicker move_picker(&pos, true, best_move, ss);
    Move picked_move;
    State st;
    NodeType type = upper_bound;

    while ((picked_move = move_picker.next_move().first) != move_none) {
        if (picked_move.flag() < knight_promotion && stand_pat + value_of(pos.piece_on[picked_move.dest()]) + 150 < alpha) {
            continue;
        }

        accumulator_stack.emplace_back(pos, picked_move, accumulator_stack.back().is_mirrored);

        uint8_t moving_piece = pos.piece_on[picked_move.src()];
        if (moving_piece >= p) moving_piece -= 6;
        (ss + 1)->piece_to = (moving_piece << 6) + picked_move.dest();

        pos.make_move(picked_move, st);
        const int score = -quiesce(pos, -beta, -alpha, ss + 1);
        accumulator_stack.pop_back();
        pos.unmake_move(picked_move);

        if (is_search_cancelled) return alpha;

        if (score > best_score) {
            best_score = score;
            best_move = picked_move;
            if (score > alpha) {
                if (score >= beta) {
                    type = lower_bound;
                    break;
                }
                alpha = score;
                type = exact;
            }
        }
    }

    TT::write(entry, pos.state->key, best_move, 0, pos.state->ply_from_root, raw_static_eval, best_score, type);

    return best_score;
}

inline int search(Position& pos, int alpha, int beta, int depth, std::list<Move>& pv, const bool cut_node, SearchEntry* ss)
{
    node_searched++;
    if (is_search_cancelled) return alpha;

    if (pos.state->ply_from_root > seldepth) {
        seldepth = pos.state->ply_from_root;
    }

    const bool not_in_check = !pos.state->checker;
    const bool is_pv = beta - alpha > 1;

    if (pos.state->ply_from_root > max_ply) {
        return not_in_check ? eval(pos) : 0;
    }

    if (pos.state->ply_from_root) {
        //Mate distance pruning
        alpha = std::max(alpha, -mate_value + pos.state->ply_from_root);
        beta = std::min(beta, mate_value - pos.state->ply_from_root - 1);
        if (alpha >= beta) return alpha;

        if (alpha < draw && pos.upcoming_repetition(pos.state->ply_from_root)) {
            alpha = draw;
            if (alpha >= beta) return alpha;
        }

        if (pos.state->repetition >= 3 || pos.state->rule_50 >= 100) return draw;

        if (!not_in_check) depth += 1;
    }

    if (depth <= 0) {
        return quiesce(pos, alpha, beta, ss);
    }

    bool match = false;
    int saved_eval;
    Move depth_best_move = move_none;
    Entry* entry = TT::probe(pos.state->key, match, pos.state->ply_from_root, saved_eval);
    uint8_t entry_type = entry->age_type & 0b11;

    if (match) {
        if (entry->depth >= depth && !is_pv) {
            if (entry_type == exact
                || (entry_type == lower_bound && saved_eval >= beta)
                || (entry_type == upper_bound && saved_eval <= alpha))
                return saved_eval;
        }
        depth_best_move = entry->best_move;
    }

    int raw_static_eval;
    int static_eval;
    if (!not_in_check) {
        raw_static_eval = score_none;
        static_eval = score_none;
    }
    else {
        if (match) {
            if (entry->static_eval != score_none) {
                raw_static_eval = entry->static_eval;
            }
            else {
                raw_static_eval = eval(pos);
            }

            static_eval = Corrections::correct(raw_static_eval, pos);

            if (!((static_eval > saved_eval && entry_type == lower_bound) || (static_eval < saved_eval && entry_type == upper_bound)))
                static_eval = saved_eval;
        }
        else {
            raw_static_eval = eval(pos);
            static_eval = Corrections::correct(raw_static_eval, pos);
        }
    }

    ss->static_eval = static_eval;
    bool improving;

    if (!not_in_check) improving = false;
    else if (int16_t two_plies_ago; (two_plies_ago = (ss - 2)->static_eval) != score_none) {
        improving = static_eval > two_plies_ago;
    }
    else if (int16_t four_plies_ago; (four_plies_ago = (ss - 4)->static_eval) != score_none) {
        improving = static_eval > four_plies_ago;
    }
    else improving = true;

    bool futility_pruning_allowed = false;

    if (not_in_check && pos.state->ply_from_root && !is_pv) {
        if (depth <= 9) {
            //Reverse futility pruning
            if (static_eval < mate_in_max_ply && static_eval - 120 * depth >= beta - improving * 70)
                return beta + (static_eval - beta) / 3;

            //Futility pruning
            if (static_eval + 140 * depth <= alpha) futility_pruning_allowed = true;
        }

        //Razoring
        if (depth <= 5) {
            if (static_eval + 130 * depth <= alpha) {
                if (quiesce(pos, alpha, beta, ss) < alpha) return alpha;
            }
        }

        //Null move pruning
        if (ss->piece_to != UINT16_MAX && depth >= 3 &&
        std::popcount(pos.occupations[2]) - std::popcount(pos.boards[P]) - std::popcount(pos.boards[p]) > 2) {
            if (static_eval >= beta) {
                const int r = std::min((static_eval - beta) / 200, 2) + depth / 5 + 3 + improving;
                State st;
                std::list<Move> local_pv;
                (ss + 1)->piece_to = UINT16_MAX;

                pos.make_null_move(st);
                const int null_score = -search(pos, -beta, -beta + 1, depth - r, local_pv, !cut_node, ss + 1);
                pos.unmake_null_move();

                if (is_search_cancelled) return alpha;
                if (null_score >= beta) return beta;
            }
        }
    }

    std::forward_list<Move> quiets_searched;
    std::forward_list<CaptureEntry> capture_searched;

    uint8_t move_searched = 0;
    int best_score = negative_infinity;
    NodeType type = upper_bound;
    std::pair<Move, int16_t> picked;
    MovePicker move_picker(&pos, false, depth_best_move, ss);
    
    if ((is_pv || cut_node) && move_picker.pv == move_none && depth >= 3) depth -= 1;
    const int late_move_margin = 5 + depth * depth - improving * 2 * depth / 3;

    while ((picked = move_picker.next_move()).first != move_none) {
        auto [picked_move, picked_score] = picked;

        //Late move pruning
        if (pos.state->ply_from_root && move_searched >= late_move_margin && depth <= 4 && not_in_check && move_picker.stage >= quiet_moves && abs(mate_value) - abs(best_score) > 128) break;
        
        if (pos.state->ply_from_root && best_score > -mate_in_max_ply && picked_score < INT16_MAX && move_picker.stage == quiet_moves) {
            if (futility_pruning_allowed) {
                move_picker.current = move_picker.moves.last;
                continue;
            }

            //History pruning
            if (depth <= 7 && picked_score < -610 * depth) continue;
        }

        std::list<Move> local_pv;
        accumulator_stack.emplace_back(pos, picked_move, accumulator_stack.back().is_mirrored);

        int score;
        State st;

        uint8_t moving_piece = pos.piece_on[picked_move.src()];
        if (moving_piece >= p) moving_piece -= 6;
        (ss + 1)->piece_to = (moving_piece << 6) + picked_move.dest();
        pos.make_move(picked_move, st);

        if (move_searched == 0) {
            score = -search(pos, -beta, -alpha, depth - 1, local_pv, false, ss + 1);
        }
        else {
            //Late move reductions
            int reduction = 1;

            if (depth >= 2 && move_searched >= 1 + 3 * is_pv) {
                reduction += reductions[depth - 1][move_searched - 1];
                reduction += cut_node;
                reduction += !is_pv;

                if (picked_score == INT16_MAX)
                    reduction -= 1;
                else if (move_picker.stage == quiet_moves) reduction -= std::clamp(picked_score / 4096, -3, 3);

                reduction = std::clamp(reduction, 1, depth - 1);
            }

            score = -search(pos, -alpha - 1, -alpha, depth - reduction, local_pv, !cut_node, ss + 1);
            if (score > alpha && beta - alpha > 1) {
                score = -search(pos, -beta, -alpha, depth - 1, local_pv, false, ss + 1);
            }
        }
        accumulator_stack.pop_back();
        pos.unmake_move(picked_move);

        if (is_search_cancelled) return alpha;
        move_searched++;

        if (score > best_score) {
            best_score = score;
            depth_best_move = picked_move;
            if (score > alpha) {
                pv.clear();
                pv.push_back(picked_move);
                pv.splice(pv.end(), local_pv);

                if (score >= beta) {
                    if (auto flag = picked_move.flag(); flag == capture || flag == ep_capture || flag >= knight_promo_capture) {
                        uint8_t captured = pos.piece_on[picked_move.dest()];
                        if (flag == ep_capture) captured = P;
                        Capture::update(capture_searched, pos.side_to_move, moving_piece , captured, picked_move.dest(), depth);
                    }
                    else {
                        Killers::insert(picked_move, pos.state->ply_from_root);
                        History::update(quiets_searched, pos.side_to_move, picked_move.src(), picked_move.dest(), depth);
                        Continuation::update(pos, quiets_searched, picked_move, depth, ss);
                    }
                    type = lower_bound;
                    break;
                }
                type = exact;
                alpha = score;
            }
        }

        if (auto flag = picked_move.flag(); flag == capture || flag == ep_capture || flag >= knight_promo_capture) {
            uint8_t captured = pos.piece_on[picked_move.dest()];
            if (flag == ep_capture) captured = P;
            if (captured >= 6) captured -= 6;

            capture_searched.emplace_front(moving_piece, captured, picked_move.dest());
        }
        else {
            quiets_searched.push_front(picked_move);
        }
    }

    if (move_searched == 0) {
        if (not_in_check) {
            return draw;
        }
        return -mate_value + pos.state->ply_from_root;
    }

    TT::write(entry, pos.state->key, depth_best_move, depth, pos.state->ply_from_root, raw_static_eval, best_score, type);

    if (not_in_check && ((depth_best_move.flag() != capture && depth_best_move.flag() < knight_promo_capture && depth_best_move.flag() != ep_capture) || depth_best_move == move_none)) {
        const int16_t delta = best_score - static_eval;
        if (!(type == lower_bound && delta < 0) && !(type == upper_bound && delta > 0)) {
            Corrections::update(delta, pos, depth);
        }
    }

    return best_score;
}

inline void start_search(int depth, const int time)
{
    if (time != - 1) {
        depth = 127;
        Timer::start(time);
    }
    else Timer::start(6000000);

    node_searched = 0;
    Move best_move = move_none;
    std::list<Move> principal_variation;

    std::array<SearchEntry, 140> search_stack;
    search_stack_init(search_stack.data());

    accumulator_stack.clear();
    accumulator_stack.emplace_back(root_accumulators, position);

    int alpha = negative_infinity;
    int beta = infinity;

    int window = pawn_weight / 2;
    for (int cr_depth = 1; cr_depth <= depth; cr_depth++) {
        if (is_search_cancelled) break;

        seldepth = 0;
        int score;

        while (true) {
            score = search(position, alpha, beta, cr_depth, principal_variation, false, search_stack.data() + 4);

            if (is_search_cancelled) break;

            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(score - window, static_cast<int>(negative_infinity));
            }
            else if (score >= beta) {
                beta = std::min(score + window, static_cast<int>(infinity));
            }
            else {
                break;
            }
            window += window / 2;
        }

        if (score == negative_infinity) continue;

        const double elapsed = Timer::elapsed();
        std::cout << "info depth " << cr_depth << " seldepth " << seldepth << " score";

        if (score < -mate_in_max_ply) std::cout << " mate " << - std::ceil((mate_value + score) / 2.0);
        else if (score > mate_in_max_ply) std::cout << " mate " << std::ceil((mate_value - score) / 2.0);
        else std::cout << " cp " << score;

        std::cout << " nodes " << node_searched
        << " nps " << std::fixed << std::setprecision(0) << node_searched / elapsed * 1000000
        << " hashfull " << TT::full()
        << " time " << elapsed / 1000
        << " pv ";

        for (auto x : principal_variation) {
            std::cout << x.get_move_string() << " ";
        }
        std::cout << std::endl;

        best_move = principal_variation.front();
    }

    Timer::stop();
    Timer::timer_thread.join();

    if (best_move == move_none) best_move = legals<all>(position).list[0];

    std::cout << "bestmove " << best_move.get_move_string() << std::endl;
}