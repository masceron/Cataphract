#pragma once

#include <algorithm>
#include <cmath>

#include "../eval/transposition.hpp"
#include "../position/position.hpp"

enum class TimeControlMode: uint8_t {
    None,
    MoveTime,
    Tournament,
};

struct TimeManager {
    Position* position = nullptr;
    double opt_time = 0.0;
    double max_time = 0.0;
    double base_opt_time = 0.0;

    int previous_score = 0;
    int stability = 0;
    Move previous_best_move = null_move;
    bool single_reply{};
    TimeControlMode mode = TimeControlMode::None;

    void init_time_control(Position* _position, const int time_left_ms, const int inc_ms, int moves_to_go = 40) {
        mode = TimeControlMode::Tournament;
        position = _position;
        {
            MoveList list;
            legals<all>(*position, list);
            single_reply = list.size() == 1;
        }

        if (moves_to_go <= 0) moves_to_go = 40;

        const double safe_time = std::max(1.0, time_left_ms - 50.0);

        base_opt_time = safe_time / moves_to_go + inc_ms;

        max_time = safe_time / std::pow(moves_to_go, 0.4) + inc_ms;

        base_opt_time = std::min(base_opt_time, safe_time);
        max_time = std::min(max_time, safe_time);

        opt_time = max_time;

        stability = 0;
        previous_score = negative_infinity;
        previous_best_move = null_move;
    }

    void init_move_time(const int move_time) {
        mode = TimeControlMode::MoveTime;

        const double safe_time = std::max(1.0, move_time - 50.0);

        base_opt_time = safe_time;
        max_time = safe_time;
        opt_time = safe_time;
    }

    void init_none() {
        mode = TimeControlMode::None;
        opt_time = 6000000;
        max_time = 6000000;
    }

    void update(const Move current_best_move, const int current_score) {
        if (mode != TimeControlMode::Tournament) {
            return;
        }

        double scale = 1.0;

        if (single_reply) {
            scale = 0.2;
        }

        if (current_best_move == previous_best_move) {
            stability++;
        } else {
            stability = 0;
            previous_best_move = current_best_move;
        }

        if (stability == 0) scale *= 2.50;
        else if (stability == 1) scale *= 1.20;
        else if (stability == 2) scale *= 0.90;
        else if (stability >= 3) scale *= 0.75;

        if (previous_score != 0) {
            if (const int score_diff = current_score - previous_score; score_diff < -30) scale *= 1.5;
            else if (score_diff > 30) scale *= 0.8;
        }
        previous_score = current_score;

        opt_time = std::min(max_time, base_opt_time * scale);
    }

    [[nodiscard]] bool should_stop(const double elapsed_ms) const {
        return elapsed_ms >= opt_time;
    }
};
