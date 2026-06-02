#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <list>
#include <print>
#include <string>
#include <forward_list>
#include <unordered_map>

#include "history.hpp"
#include "../position/movegen.hpp"
#include "movepicker.hpp"
#include "params.hpp"
#include "../position/move.hpp"
#include "see.hpp"
#include "time.hpp"
#include "timer.hpp"
#include "../eval/eval.hpp"
#include "../eval/transposition.hpp"

inline int quiesce(SearchThread& thread, int alpha, int beta, SearchEntry* ss)
{
    ++thread.node_searched;
    if (ss->plies > thread.seldepth)
    {
        thread.seldepth = ss->plies;
    }

    if (Timer::is_search_cancelled) return alpha;

    auto& position = thread.position;
    auto& accumulator_stack = thread.accumulator_stack;

    const bool not_in_check = !position.state->checker;
    if (ss->plies > MAX_PLY)
    {
        return not_in_check ? eval(position, thread.accumulator_stack) : 0;
    }

    alpha = std::max(alpha, -mate_value + ss->plies);
    beta = std::min(beta, mate_value - ss->plies - 1);
    if (alpha >= beta) return alpha;

    if (alpha < draw && position.upcoming_repetition(ss->plies))
    {
        alpha = draw;
        if (alpha >= beta) return alpha;
    }
    if (position.state->repetition >= 3 || position.state->rule_50 >= 100) return draw;

    int tt_score;
    bool tt_hit = false;
    Entry* entry;
    int16_t tt_static_eval;
    uint8_t entry_depth;
    NodeType entry_type;
    Move tt_move = null_move;
    const bool is_pv = beta - alpha > 1;

    std::tie(entry, entry_depth, entry_type, tt_move, tt_static_eval, tt_score) = TT::probe(
        position.state->key, tt_hit, ss->plies);

    int raw_static_eval;
    Move best_move = null_move;

    if (tt_hit)
    {
        if (!is_pv && (entry_type == NodeType::exact
            || (entry_type == NodeType::lower_bound && tt_score >= beta)
            || (entry_type == NodeType::upper_bound && tt_score <= alpha)))
            return tt_score;

        best_move = tt_move;
    }

    int stand_pat;

    if (!not_in_check)
    {
        stand_pat = negative_infinity;
        raw_static_eval = score_none;
    }
    else
    {
        if (tt_hit)
        {
            if (tt_static_eval != score_none)
            {
                raw_static_eval = tt_static_eval;
            }
            else
            {
                raw_static_eval = eval(position, thread.accumulator_stack);
            }
            stand_pat = thread.history.corrections.correct(raw_static_eval, position);

            if (!((stand_pat > tt_score && entry_type == NodeType::lower_bound) || (
                stand_pat < tt_score && entry_type == NodeType::upper_bound)))
            {
                stand_pat = tt_score;
            }

            best_move = tt_move;
        }
        else
        {
            raw_static_eval = eval(position, accumulator_stack);
            stand_pat = thread.history.corrections.correct(raw_static_eval, position);
        }

        if (stand_pat >= beta) return stand_pat;
        if (stand_pat > alpha) alpha = stand_pat;
    }

    int best_score = stand_pat;

    MovePicker move_picker(thread, not_in_check, best_move, ss);
    Move picked_move;
    State st;
    auto type = NodeType::upper_bound;
    std::forward_list<CaptureEntry> capture_searched;
    int move_searched = 0;

    while ((picked_move = move_picker.next_move().first))
    {
        if (not_in_check && picked_move.flag() < MoveFlag::knight_promotion)
        {
            const int captured_val = picked_move.flag() == MoveFlag::ep_capture
                                         ? value_of(P)
                                         : value_of(position.piece_on[picked_move.to()]);
            if (stand_pat + captured_val + delta_margin() < alpha)
                continue;
        }

        uint8_t moving_piece = position.piece_on[picked_move.from()];
        if (moving_piece >= p) moving_piece -= 6;
        ss->piece_to = (moving_piece << 6) + picked_move.to();

        position.make_move(picked_move, st);
        thread.accumulator_stack.push(position, picked_move);

        const int score = -quiesce(thread, -beta, -alpha, ss + 1);

        accumulator_stack.pop();
        position.unmake_move(picked_move);

        if (Timer::is_search_cancelled) return alpha;

        move_searched++;

        if (score > best_score)
        {
            best_score = score;
            best_move = picked_move;
            if (score > alpha)
            {
                if (score >= beta)
                {
                    if (const uint8_t captured = position.piece_on[picked_move.to()]; captured != nil)
                    {
                        thread.history.capture.update(capture_searched, position.side_to_move, moving_piece,
                                                      captured,
                                                      picked_move.to(), 1);
                    }
                    else if (picked_move.flag() == MoveFlag::ep_capture)
                    {
                        thread.history.capture.update(capture_searched, position.side_to_move, moving_piece, P,
                                                      picked_move.to(), 1);
                    }

                    type = NodeType::lower_bound;
                    break;
                }
                alpha = score;
                type = NodeType::exact;
            }
        }

        if (uint8_t captured = position.piece_on[picked_move.to()]; captured != nil)
        {
            capture_searched.emplace_front(moving_piece, captured, picked_move.to());
        }
        else if (picked_move.flag() == MoveFlag::ep_capture)
        {
            capture_searched.emplace_front(moving_piece, P, picked_move.to());
        }
    }

    if (!move_searched && !not_in_check)
    {
        return -mate_value + ss->plies;
    }

    TT::write(entry, position.state->key, best_move, 0, ss->plies, raw_static_eval, best_score, type, is_pv);

    return best_score;
}

template <const bool root_node, const bool is_pv>
int search(SearchThread& thread, int alpha, int beta, int depth, std::list<Move>& pv, const bool cut_node,
           SearchEntry* ss)
{
    ++thread.node_searched;
    if (Timer::is_search_cancelled) return alpha;

    if (ss->plies > thread.seldepth)
    {
        thread.seldepth = ss->plies;
    }

    auto& position = thread.position;
    auto& accumulator_stack = thread.accumulator_stack;

    const bool not_in_check = !position.state->checker;

    if (ss->plies > MAX_PLY)
    {
        return not_in_check ? eval(position, accumulator_stack) : 0;
    }

    if constexpr (!root_node)
    {
        alpha = std::max(alpha, -mate_value + ss->plies);
        beta = std::min(beta, mate_value - ss->plies - 1);
        if (alpha >= beta) return alpha;

        if (alpha < draw && position.upcoming_repetition(ss->plies))
        {
            alpha = draw;
            if (alpha >= beta) return alpha;
        }

        if (position.state->repetition >= 3 || position.state->rule_50 >= 100) return draw;

        if (!not_in_check) depth += 1;
    }

    if (depth <= 0)
    {
        return quiesce(thread, alpha, beta, ss);
    }

    bool tt_hit = false;
    int tt_score = score_none;
    Move depth_best_move = null_move;
    Move tt_move = null_move;
    int tt_depth;
    Entry* entry;
    NodeType entry_type;
    ss->double_extensions = (ss - 1)->double_extensions;
    uint64_t tt_key = position.state->key ^ static_cast<uint64_t>(ss->excluded.move) << 16;

    int raw_static_eval;
    bool improving = false;

    int16_t tt_static_eval;

    std::tie(entry, tt_depth, entry_type, tt_move, tt_static_eval, tt_score) = TT::probe(tt_key, tt_hit, ss->plies);

    if (tt_hit)
    {
        if (tt_depth >= depth && !is_pv)
        {
            if (entry_type == NodeType::exact
                || (entry_type == NodeType::lower_bound && tt_score >= beta)
                || (entry_type == NodeType::upper_bound && tt_score <= alpha))
            {
                if (tt_score >= beta && tt_move)
                {
                    position.fill_info();
                    if (position.is_pseudo_legal(tt_move) && position.is_quiet(tt_move) && position.is_legal(tt_move))
                    {
                        thread.history.update_quiet_histories(position, depth, tt_move, ss, {});
                    }
                }

                return tt_score;
            }
        }
    }

    if (!not_in_check)
    {
        raw_static_eval = score_none;
        ss->static_eval = score_none;
    }
    else
    {
        if (tt_hit)
        {
            if (tt_static_eval != score_none)
            {
                raw_static_eval = tt_static_eval;
            }
            else
            {
                raw_static_eval = eval(position, accumulator_stack);
            }

            ss->static_eval = thread.history.corrections.correct(raw_static_eval, position);

            if (!((ss->static_eval > tt_score && entry_type == NodeType::lower_bound) || (
                ss->static_eval < tt_score && entry_type == NodeType::upper_bound)))
                ss->static_eval = tt_score;
        }
        else
        {
            raw_static_eval = eval(position, accumulator_stack);
            ss->static_eval = thread.history.corrections.correct(raw_static_eval, position);
        }

        if (int two_plies_ago; (two_plies_ago = (ss - 2)->static_eval) != score_none)
        {
            improving = ss->static_eval > two_plies_ago;
        }
        else if (int16_t four_plies_ago; (four_plies_ago = (ss - 4)->static_eval) != score_none)
        {
            improving = ss->static_eval > four_plies_ago;
        }
        else improving = true;
    }

    bool futility_pruning_allowed = false;

    if (not_in_check && !is_pv)
    {
        if (depth <= 9 && !ss->excluded)
        {
            if (ss->static_eval < mate_in_max_ply && ss->static_eval - futility_cutoff_scale() * depth >= beta -
                improving * futility_cutoff_scale_imp())
                return beta + (ss->static_eval - beta) / 3;

            if (ss->static_eval + futility_scale() * depth <= alpha) futility_pruning_allowed = true;
        }

        if (depth <= 5)
        {
            if (ss->static_eval + razoring_scale() * depth <= alpha)
            {
                if (quiesce(thread, alpha, beta, ss) < alpha) return alpha;
            }
        }

        if ((ss - 1)->piece_to != UINT16_MAX && depth >= 3 && ss->plies >= thread.nmp_min_ply &&
            std::popcount(position.occupations[2]) - std::popcount(position.boards[P]) - std::popcount(
                position.boards[p]) > 2 &&
            !ss->excluded)
        {
            if (ss->static_eval >= beta)
            {
                const int r = std::min((ss->static_eval - beta) / null_search_div(), 2) + depth *
                    null_search_depth_scale() / 1024 + 3 + improving;
                State st;
                std::list<Move> local_pv;
                ss->piece_to = UINT16_MAX;

                position.make_null_move(st);
                const int null_score = -search<false, false>(thread, -beta, -beta + 1, depth - r, local_pv, !cut_node,
                                                             ss + 1);
                position.unmake_null_move();

                if (Timer::is_search_cancelled) return alpha;
                if (null_score >= beta && std::abs(null_score) < mate_in_max_ply)
                {
                    if (thread.nmp_min_ply > 0 || depth < 16)
                    {
                        return null_score;
                    }

                    thread.nmp_min_ply = ss->plies + 3 * (depth - r) / 4;

                    const auto verification_score = search<false, false>(
                        thread, beta - 1, beta, depth - r, local_pv, !cut_node, ss + 1);

                    thread.nmp_min_ply = 0;

                    if (Timer::is_search_cancelled) return alpha;

                    if (verification_score >= beta) return null_score;
                }
            }
        }

        const int prob_beta = beta + probcut_margin() - probcut_scale() * improving;
        if (const int prob_depth = std::max(depth - 3, 1);
            depth >= 6 && std::abs(beta) < mate_in_max_ply &&
            !(tt_hit && tt_depth >= prob_depth && tt_score < prob_beta))
        {
            MovePicker prob_picker(thread, true, tt_move, ss, prob_beta - ss->static_eval);
            std::pair<Move, int> picked;

            while ((picked = prob_picker.next_move()).first)
            {
                auto [picked_move, picked_score] = picked;

                if (picked_move == ss->excluded) continue;

                uint8_t moving_piece = position.piece_on[picked_move.from()];
                if (moving_piece >= p) moving_piece -= 6;
                ss->piece_to = (moving_piece << 6) + picked_move.to();

                State st;

                position.make_move(picked_move, st);
                accumulator_stack.push(position, picked_move);

                int prob_score = -quiesce(thread, -prob_beta, -prob_beta + 1, ss + 1);

                if (prob_score >= prob_beta)
                {
                    std::list<Move> temp;
                    prob_score = -search<false, false>(thread, -prob_beta, -prob_beta + 1, prob_depth - 1, temp,
                                                       !cut_node,
                                                       ss + 1);
                }

                accumulator_stack.pop();
                position.unmake_move(picked_move);

                if (prob_score >= prob_beta)
                {
                    TT::write(entry, tt_key, picked_move, prob_depth, ss->plies, raw_static_eval, prob_score,
                              NodeType::lower_bound, is_pv);
                    return prob_score;
                }
            }
        }
    }

    std::forward_list<Move> quiets_searched;
    std::forward_list<CaptureEntry> capture_searched;

    int move_searched = 0;
    int best_score = negative_infinity;
    NodeType type = NodeType::upper_bound;
    std::pair<Move, int> picked;
    MovePicker move_picker(thread, false, tt_move, ss);

    if (not_in_check && (is_pv || cut_node) && !move_picker.pv && depth >= 3 && !ss->excluded) depth--;
    const int late_move_margin = lmp[improving][std::min(depth - 1, 15)];

    while ((picked = move_picker.next_move()).first)
    {
        auto [picked_move, picked_score] = picked;

        if (picked_move == ss->excluded) continue;

        ++move_searched;

        int extension = 0;
        if (move_picker.stage == Stage::quiet_moves && !root_node)
        {
            if (not_in_check && move_searched >= late_move_margin && std::abs(best_score) < mate_in_max_ply)
            {
                move_picker.skip_quiets();
                continue;
            }
            if (picked_score < INT16_MAX && best_score > -mate_in_max_ply)
            {
                if (futility_pruning_allowed)
                {
                    move_picker.skip_quiets();
                    continue;
                }
                if (depth <= 7 && picked_score < -history_prune_scale() * depth) continue;
            }
        }

        if constexpr (root_node)
        {
            if (&thread == &ThreadPool::get(0) && Options::show_currmove && Timer::elapsed() / 1000 > 2500)
            {
                std::println("info depth {} currmove {} currmovenumber {}", thread.root_depth,
                             picked_move.get_move_string(), move_searched);
            }
        }

        if (tt_hit && !root_node && !ss->excluded && ss->plies < 2 * thread.root_depth && ss->double_extensions < 2 *
            thread.root_depth)
        {
            if (depth >= 8 && picked_move == tt_move && tt_depth >= depth - 3 && entry_type != NodeType::upper_bound &&
                std::abs(tt_score) < mate_in_max_ply)
            {
                const auto singular_beta = std::max(tt_score - singular_margin() * depth / 1024, -mate_in_max_ply);
                const auto singular_depth = (depth - 1) / 2;
                std::list<Move> tmp_pv;

                ss->excluded = tt_move;
                const auto singular_score = search<false, false>(thread, singular_beta - 1, singular_beta,
                                                                 singular_depth, tmp_pv,
                                                                 cut_node, ss);
                ss->excluded = null_move;

                if (singular_score < singular_beta)
                {
                    if (!is_pv && picked_move.is_quiet())
                    {
                        if (singular_beta - singular_score > singular_triple() && ss->double_extensions <= 5)
                        {
                            extension = 3;
                            ss->double_extensions = (ss - 1)->double_extensions + 1;
                        }
                        else if (singular_beta - singular_score > singular_double() && ss->double_extensions <= 4)
                        {
                            extension = 2;
                            ss->double_extensions = (ss - 1)->double_extensions + 1;
                        }
                        else extension = 1;
                    }
                    else
                    {
                        extension = 1;
                    }
                }
                else if (singular_beta >= beta)
                {
                    return singular_beta;
                }
                else if (tt_score >= beta) extension = -2 + is_pv;
                else if (cut_node) extension = -2;
                else if (tt_score <= alpha) extension = -1;
            }
        }

        std::list<Move> local_pv;
        int score;
        State st;

        uint8_t moving_piece = position.piece_on[picked_move.from()];
        if (moving_piece >= p) moving_piece -= 6;
        ss->piece_to = (moving_piece << 6) + picked_move.to();

        position.make_move(picked_move, st);
        accumulator_stack.push(position, picked_move);

        const auto new_depth = depth + extension;

        if (depth >= 2 && move_searched >= 2 + root_node)
        {
            int reduction = 1;

            reduction += reductions[depth - 1][std::min(move_searched - 1, 62)];
            reduction += cut_node;
            reduction += !is_pv;
            reduction += !improving;

            reduction -= tt_hit && tt_depth >= depth;
            if (picked_score == INT16_MAX)
                reduction -= 2;
            else if (move_picker.stage == Stage::quiet_moves)
                reduction -= std::clamp(picked_score / history_prune_div(), -3, 3);

            reduction = std::clamp(reduction, 1, new_depth - 1);

            score = -search<false, false>(thread, -alpha - 1, -alpha, new_depth - reduction, local_pv, true, ss + 1);

            if (score > alpha && reduction > 1)
            {
                score = -search<false, false>(thread, -alpha - 1, -alpha, new_depth - 1, local_pv, !cut_node, ss + 1);
            }
        }
        else if (!is_pv || move_searched)
        {
            score = -search<false, false>(thread, -alpha - 1, -alpha, new_depth - 1, local_pv, !cut_node, ss + 1);
        }

        if (is_pv && (!move_searched || score > alpha))
        {
            score = -search<false, true>(thread, -beta, -alpha, new_depth - 1, local_pv, false, ss + 1);
        }

        accumulator_stack.pop();
        position.unmake_move(picked_move);

        if (Timer::is_search_cancelled) return alpha;

        if (score > best_score)
        {
            best_score = score;
            depth_best_move = picked_move;
            if (score > alpha)
            {
                pv.clear();
                pv.push_back(picked_move);
                pv.splice(pv.end(), local_pv);

                if (score >= beta)
                {
                    if (position.is_quiet(picked_move))
                    {
                        thread.history.update_quiet_histories(position, depth, picked_move, ss, quiets_searched);
                    }
                    else
                    {
                        if (uint8_t captured = position.piece_on[picked_move.to()]; captured != nil)
                        {
                            thread.history.capture.update(capture_searched, position.side_to_move, moving_piece,
                                                          captured,
                                                          picked_move.to(),
                                                          depth);
                        }
                        else if (picked_move.flag() == MoveFlag::ep_capture)
                        {
                            thread.history.capture.update(capture_searched, position.side_to_move, moving_piece, P,
                                                          picked_move.to(),
                                                          depth);
                        }
                    }
                    type = NodeType::lower_bound;
                    break;
                }
                type = NodeType::exact;
                alpha = score;
            }
        }

        if (position.is_quiet(picked_move))
        {
            quiets_searched.push_front(picked_move);
        }
        else
        {
            if (uint8_t captured = position.piece_on[picked_move.to()]; captured != nil)
            {
                capture_searched.emplace_front(moving_piece, captured, picked_move.to());
            }
            else if (picked_move.flag() == MoveFlag::ep_capture)
            {
                capture_searched.emplace_front(moving_piece, P, picked_move.to());
            }
        }
    }

    if (move_searched == 0)
    {
        if (ss->excluded) return alpha;
        if (not_in_check)
        {
            return draw;
        }
        return -mate_value + ss->plies;
    }

    TT::write(entry, tt_key, depth_best_move, depth, ss->plies, raw_static_eval, best_score, type, is_pv);

    if (!ss->excluded)
    {
        if (not_in_check && ((depth_best_move.flag() != MoveFlag::capture && depth_best_move.flag() <
            MoveFlag::knight_promo_capture &&
            depth_best_move.flag() != MoveFlag::ep_capture) || !depth_best_move))
        {
            if (const auto delta = best_score - ss->static_eval;
                !(type == NodeType::lower_bound && delta < 0) && !(type == NodeType::upper_bound && delta > 0))
            {
                thread.history.corrections.update(delta, position, depth);
            }
        }
    }

    return best_score;
}

inline void print_info(const SearchThread& thread)
{
    const auto elapsed = Timer::elapsed();
    std::print("info depth {} seldepth {} score ", thread.root_depth, thread.seldepth);

    if (thread.score < -mate_in_max_ply) std::print("mate {} ", -std::ceil((mate_value + thread.score) / 2.0));
    else if (thread.score > mate_in_max_ply)
        std::print(
            "mate {} ", std::ceil((mate_value - thread.score) / 2.0));
    else std::print("cp {} ", thread.score);

    auto node_searched = ThreadPool::node_searched();

    std::print("nodes {} nps {} hashfull {} time {} pv ",
               node_searched,
               static_cast<uint64_t>(static_cast<double>(node_searched) / elapsed * 1000000),
               TT::full(),
               elapsed / 1000);

    for (auto x : thread.principal_variation)
    {
        std::print("{} ", x.get_move_string());
    }
    std::println();
    std::fflush(stdout);
}

template <bool silent>
void thread_search(const int thread_idx, const int search_depth)
{
    auto& thread = ThreadPool::get(thread_idx);
    int previous_score = negative_infinity;

    Move best_move = null_move;
    auto& principal_variation = thread.principal_variation;

    thread.search_stack_init();

    for (thread.root_depth = 1; thread.root_depth <= search_depth; thread.root_depth++)
    {
        if (Timer::is_search_cancelled) break;

        thread.seldepth = 0;
        int fail_high_reductions = 0;

        int alpha = negative_infinity;
        int beta = infinity;
        int window = initial_aspiration_window();

        if (thread.root_depth >= 3)
        {
            alpha = std::max(previous_score - window, static_cast<int>(negative_infinity));
            beta = std::min(previous_score + window, static_cast<int>(infinity));
        }

        while (true)
        {
            const int new_depth = std::max(thread.root_depth - fail_high_reductions, 1);
            thread.score = search<true, true>(thread, alpha, beta, new_depth, principal_variation, false,
                                              &thread.search_stack[4]);

            if (Timer::is_search_cancelled) break;

            if (thread.score <= alpha)
            {
                fail_high_reductions = 0;
                beta = (alpha + beta) / 2;
                alpha = std::max(thread.score - window, static_cast<int>(negative_infinity));
            }
            else if (thread.score >= beta)
            {
                beta = std::min(thread.score + window, static_cast<int>(infinity));
                fail_high_reductions = std::min(fail_high_reductions + 1, 3);
            }
            else
            {
                previous_score = thread.score;
                break;
            }
            window += window * aspiration_expansion_rate() / 128;
        }
        if (Timer::is_search_cancelled) break;

        if (!silent && thread_idx == 0)
        {
            print_info(thread);
        }

        if (!principal_variation.empty())
        {
            best_move = principal_variation.front();
        }

        if (thread_idx == 0)
        {
            time_manager.update(best_move, thread.score);

            if (const double elapsed_ms = static_cast<double>(Timer::elapsed()) / 1000.0; time_manager.should_stop(
                elapsed_ms))
            {
                break;
            }
        }
        else if (thread.root_depth == search_depth) --thread.root_depth;
    }
}

inline SearchThread& thread_vote()
{
    if (Options::threads == 1) return ThreadPool::get(0);

    int lowest_score = infinity;

    std::unordered_map<Move, int> votes;

    for (const auto& thread : ThreadPool::threads)
    {
        if (const auto score = thread.score; score != negative_infinity && score < lowest_score) lowest_score = score;

        if (!thread.principal_variation.empty())
        {
            votes[thread.principal_variation.front()] = 0;
        }
    }

    const auto weight = [&](const SearchThread& thread)
    {
        return (thread.score - lowest_score + 10) * thread.root_depth;
    };

    for (auto& thread : ThreadPool::threads)
    {
        auto principal = thread.principal_variation.front();
        if (thread.score != negative_infinity)
        {
            votes[principal] += weight(thread);
        }
    }

    SearchThread* best_thread = &ThreadPool::get(0);
    int best_score = best_thread->score;
    int best_votes = votes[best_thread->principal_variation.front()];

    for (auto i = 1; i < Options::threads; i++)
    {
        auto& thread = ThreadPool::get(i);
        const int score = thread.score;
        const int vote = votes[thread.principal_variation.front()];

        if (std::abs(best_score) > mate_in_max_ply)
        {
            if (score > best_score)
            {
                best_thread = &thread;
                best_score = score;
                best_votes = vote;
            }
            continue;
        }

        if (std::abs(score) > mate_in_max_ply)
        {
            best_thread = &thread;
            best_score = score;
            best_votes = vote;
            continue;
        }

        if (vote > best_votes ||
            (vote == best_votes &&
                weight(thread) * (thread.principal_variation.size() > 2) >
                weight(*best_thread) * (best_thread->principal_variation.size() > 2)))
        {
            best_thread = &thread;
            best_score = score;
            best_votes = vote;
        }
    }

    return *best_thread;
}

template <bool silent>
void start_search(const int depth_param, const int move_time, const int wtime, const int btime,
                  const int winc, const int binc, const int moves_to_go)
{
    std::vector<std::jthread> searchers;
    searchers.reserve(Options::threads);

    time_manager = {};
    ThreadPool::prepare();

    int search_depth = MAX_PLY;

    const auto& position = ThreadPool::get(0).position;
    if (move_time > 0)
    {
        time_manager.init_move_time(move_time);
    }
    else if (depth_param > 0 && depth_param <= MAX_PLY)
    {
        time_manager.init_none();
        search_depth = depth_param;
    }
    else if (wtime > 0 || btime > 0)
    {
        const int time_left = position.side_to_move == white ? wtime : btime;
        const int increment = position.side_to_move == white ? winc : binc;

        time_manager.init_time_control(position, time_left, increment, moves_to_go);
    }

    Timer::start(static_cast<uint32_t>(time_manager.max_time));

    ThreadPool::current_search_depth = search_depth;
    ThreadPool::start_workers(WorkerTask::Search);

    thread_search<silent>(0, search_depth);

    Timer::stop();
    ThreadPool::wait_for_workers();

    Timer::timer_thread.join();

    if constexpr (!silent)
    {
        const auto& best_thread = thread_vote();
        const auto& principal_variation = best_thread.principal_variation;
        Move best_move;

        if (principal_variation.empty())
        {
            MoveList list;
            legals<MoveType::all>(best_thread.position, list);
            best_move = list[0];
        }
        else best_move = principal_variation.front();

        if (best_thread.id)
        {
            print_info(best_thread);
        }

        if (Options::show_currmove)
        {
            std::println("info string Selected thread {}", best_thread.id);
        }

        std::println("bestmove {}", best_move.get_move_string());
        std::fflush(stdout);
    }
}
