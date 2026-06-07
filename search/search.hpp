#pragma once

template <bool silent>
void thread_search(int thread_idx, int search_depth);

template <bool silent>
void start_search(int depth_param, int move_time, int wtime, int btime, int winc, int binc, int moves_to_go);