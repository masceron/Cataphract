#include "movepicker.hpp"
#include "params.hpp"
#include "see.hpp"

constexpr int16_t mvv[14] = {
    105, 205, 305, 405, 505, 605, 0, 0, 105, 205, 305, 405, 505, 605
};

MovePicker::MovePicker(SearchThread& _thread, const bool _noisy_only, const Move _pv, SearchEntry* _ss,
                       const int _threshold) : thread(_thread), ss(_ss), noisy_only(_noisy_only)
{
    bad_captures_end = 255;
    threshold = _threshold;

    thread.position.fill_info();

    if (const auto pv_flag = pv.flag(); _pv && thread.position.is_pseudo_legal(_pv)
        && !(noisy_only
            && pv_flag != MoveFlag::queen_promotion
            && pv_flag != MoveFlag::capture
            && pv_flag < MoveFlag::knight_promo_capture
            && pv_flag != MoveFlag::ep_capture))
    {
        pv = _pv;
        stage = Stage::TT_moves;
    }
}

std::pair<Move, int> MovePicker::pick()
{
    const auto& pos = thread.position;
    const auto& history = thread.history;

    switch (stage)
    {
    case Stage::TT_moves:
        stage = Stage::generating_capture_moves;
        return {pv, 0};

    case Stage::generating_capture_moves:
        stage = Stage::good_capture_moves;
        pseudo_legals<MoveType::noisy>(pos, moves);
        end = moves.last - moves.begin();
        score_mvv_caphist(0, end);
        current = 0;
        [[fallthrough]];

    case Stage::good_capture_moves:
        while (current < end)
        {
            select_highest(current, end);
            Move move = moves.list[current];
            auto score = scores[current];
            current++;

            if (move == pv) continue;

            if (const auto exc = static_exchange_evaluation(thread.position, move); exc < threshold)
            {
                moves.list[bad_captures_end] = move;
                scores[bad_captures_end] = exc;
                bad_captures_end--;
                continue;
            }

            return {move, score};
        }
        stage = Stage::killer_1;
        [[fallthrough]];

    case Stage::killer_1:
        stage = Stage::killer_2;
        if (const Move killer = history.killers.get(ss->plies, 0); killer != pv && thread.position.
            is_pseudo_legal(killer))
        {
            return {killer, UINT16_MAX};
        }
        [[fallthrough]];

    case Stage::killer_2:
        stage = Stage::generating_quiet_moves;
        if (const Move killer = history.killers.get(ss->plies, 1); killer != pv && thread.position.
            is_pseudo_legal(killer))
        {
            return {killer, UINT16_MAX};
        }
        [[fallthrough]];

    case Stage::generating_quiet_moves:
        if (!noisy_only)
        {
            current = end;
            pseudo_legals<MoveType::quiet>(thread.position, moves);
            end = moves.last - moves.begin();
            score_history(current, end);
            stage = Stage::quiet_moves;
        }
        else
        {
            stage = Stage::none;
            return {null_move, 0};
        }
        [[fallthrough]];

    case Stage::quiet_moves:
        while (current < end)
        {
            select_highest(current, end);
            Move move = moves.list[current];
            auto score = scores[current];
            current++;

            if (move == pv ||
                move == history.killers.get(ss->plies, 0) ||
                move == history.killers.get(ss->plies, 1))
            {
                continue;
            }

            return {move, score};
        }

        stage = Stage::bad_capture_moves;
        current = bad_captures_end + 1;
        end = 256;
        [[fallthrough]];

    case Stage::bad_capture_moves:
        while (current < end)
        {
            select_highest(current, end);
            Move move = moves.list[current];
            auto score = scores[current];
            current++;
            return {move, score};
        }
        stage = Stage::none;
        [[fallthrough]];

    case Stage::none: default:
        return {null_move, 0};
    }
}

void MovePicker::skip_quiets()
{
    current = end;
}

std::pair<Move, int> MovePicker::next_move()
{
    auto move = pick();
    if (!move.first) return {null_move, 0};
    const auto& pos = thread.position;

    while (!pos.is_legal(move.first))
    {
        move = pick();
        if (!move.first) return {null_move, 0};
    }
    return move;
}

void MovePicker::score_mvv_caphist(const int start_idx, const int end_idx)
{
    if (start_idx == end_idx) return;

    const auto& pos = thread.position;
    const auto& history = thread.history;
    const bool stm = pos.side_to_move;

    for (int i = start_idx; i < end_idx; ++i)
    {
        Move move = moves.list[i];
        const auto from = move.from();
        const auto to = move.to();

        const int moved = pos.piece_on[from] & 7;
        int captured = pos.piece_on[to];

        if (captured == nil) captured = P;
        captured &= 7;

        scores[i] = mvv[captured] * mvv_weight() / 1024 + history.capture.table[stm][moved][captured][to] *
            capture_history_weight() / 1024;

        if (move.flag() >= MoveFlag::knight_promo_capture)
            scores[i] = scores[i] + value_of(move.promoted_to());
    }
}

void MovePicker::score_history(int start_idx, int end_idx)
{
    if (start_idx == end_idx) return;

    const auto prev = (ss - 1)->piece_to != UINT16_MAX ? (ss - 1)->piece_to : 0;
    const auto prev2 = (ss - 2)->piece_to != UINT16_MAX ? (ss - 2)->piece_to : 0;
    const auto prev4 = (ss - 4)->piece_to != UINT16_MAX ? (ss - 4)->piece_to : 0;
    const auto& pos = thread.position;
    const auto& history = thread.history;

    for (int i = start_idx; i < end_idx; ++i)
    {
        Move move = moves.list[i];
        const auto from = move.from();
        const auto to = move.to();

        const int moved = pos.piece_on[from] & 7;

        scores[i] =
            (history.butterfly_history.table[pos.side_to_move][from][to]
                + history.piece_to_history.table[pos.side_to_move][moved][to]) * piece_history_weight() / 1024
            + history.continuation.counter_moves[pos.side_to_move][prev >> 6 & 7][prev & 0b111111][moved][to] *
            counter_move_weight() / 1024
            + history.continuation.follow_up[pos.side_to_move][prev2 >> 6 & 7][prev2 & 0b111111][moved][to] *
            follow_up_weight() / 1024
            + history.continuation.four_plies[pos.side_to_move][prev4 >> 6 & 7][prev4 & 0b111111][moved][to] *
            four_plies_weight() / 1024;
    }
}

void MovePicker::select_highest(int start_idx, int end_idx)
{
    int best_idx = start_idx;
    auto best_score = scores[start_idx];

    for (int i = start_idx + 1; i < end_idx; ++i)
    {
        if (const auto score = scores[i]; score > best_score)
        {
            best_score = score;
            best_idx = i;
        }
    }

    std::swap(moves.list[start_idx], moves.list[best_idx]);
    std::swap(scores[start_idx], scores[best_idx]);
}
