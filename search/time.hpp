#pragma once

#include <algorithm>
#include <cmath>

#include "../eval/transposition.hpp"
#include "../position/position.hpp"

enum class TimeControlMode: uint8_t
{
    None,
    MoveTime,
    Tournament,
};

struct TimeManager
{
    Position* position = nullptr;
    double opt_time = 0.0;
    double max_time = 0.0;
    double scale = 1.0;

    int previous_score = negative_infinity;
    int stability = 0;
    Move previous_best_move = null_move;
    bool single_reply{};
    TimeControlMode mode = TimeControlMode::None;

    void init_time_control(Position* _position, const int time_left_ms, const int inc_ms, int moves_to_go)
    {
        mode = TimeControlMode::Tournament;
        position = _position;
        {
            MoveList list;
            legals<all>(*position, list);
            single_reply = list.size() == 1;
        }

        if (moves_to_go <= 0) moves_to_go = default_moves_to_go();

        const double safe_time_ms = std::max(1.0, time_left_ms - 50.0);

        const auto base_opt_time = safe_time_ms / moves_to_go + inc_ms * 0.75;

        max_time = safe_time_ms * max_time_scale();
        opt_time = std::min(base_opt_time * opt_time_scale(), max_time);
    }

    void init_move_time(const int move_time)
    {
        mode = TimeControlMode::MoveTime;

        const double safe_time = std::max(1.0, move_time - 50.0);

        max_time = safe_time;
        opt_time = safe_time;
    }

    void init_none()
    {
        mode = TimeControlMode::None;
        opt_time = 6000000;
        max_time = 6000000;
    }

    void update(const Move current_best_move, const int current_score)
    {
        if (mode != TimeControlMode::Tournament)
        {
            return;
        }

        double tmp_scale = 1.0;

        if (single_reply)
        {
            tmp_scale = 0.15;
        }
        else
        {
            if (current_best_move == previous_best_move)
            {
                stability = std::min(stability + 1, 10);
            }
            else
            {
                stability = 0;
                previous_best_move = current_best_move;
            }

            tmp_scale *= stable_base_scale() - static_cast<float>(stability) * stable_scale();

            if (previous_score != negative_infinity)
            {
                tmp_scale *= std::pow(
                    2.0, static_cast<double>(std::clamp(previous_score - current_score, -score_diff_scale(),
                                                        score_diff_scale())) / score_diff_scale());
            }
        }

        previous_score = current_score;
        scale = tmp_scale;
    }

    [[nodiscard]] bool should_stop(const double elapsed_ms) const
    {
        return elapsed_ms >= std::min(max_time, opt_time * scale);
    }
};
