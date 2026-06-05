#pragma once

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
    double opt_time = 0.0;
    double max_time = 0.0;
    double scale = 1.0;

    int previous_score = negative_infinity;
    int stability = 0;
    Move previous_best_move = null_move;
    bool single_reply{};
    TimeControlMode mode = TimeControlMode::None;

    void init_time_control(const Position& _position, int time_left_ms, int inc_ms, int moves_to_go);
    void init_move_time(int move_time);
    void init_none();
    void update(Move current_best_move, int current_score);
    [[nodiscard]] bool should_stop(double elapsed_ms) const;
};

inline TimeManager time_manager;