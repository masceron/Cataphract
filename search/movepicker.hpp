#pragma once

#include "thread.hpp"
#include "../position/movegen.hpp"

enum class Stage: uint8_t
{
    TT_moves,
    generating_capture_moves,
    good_capture_moves,
    killer_1,
    killer_2,
    generating_quiet_moves,
    quiet_moves,
    bad_capture_moves,
    none,
};

struct MovePicker
{
    MoveList moves;
    SearchThread& thread;
    SearchEntry* ss;
    Move pv = null_move;

    int current = 0;
    int end = 0;
    int bad_captures_end = 255;

    int threshold;
    int scores[256];
    Stage stage = Stage::generating_capture_moves;
    bool noisy_only;

    explicit MovePicker(SearchThread& _thread, bool _noisy_only, Move _pv, SearchEntry* _ss, int _threshold = 0);
    std::pair<Move, int> pick();
    void skip_quiets();
    std::pair<Move, int> next_move();
    void score_mvv_caphist(int start_idx, int end_idx);
    void score_history(int start_idx, int end_idx);
    void select_highest(int start_idx, int end_idx);
};
