#pragma once

template <bool silent>
void thread_search(const int thread_idx, const int search_depth);

template <bool silent>
void start_search(const int depth_param, const int move_time, const int wtime, const int btime,
                  const int winc, const int binc, const int moves_to_go);