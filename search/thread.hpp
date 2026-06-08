#pragma once

#include <vector>
#include <condition_variable>
#include <thread>
#include <list>
#include <deque>

#include "history.hpp"
#include "../eval/accumulators.hpp"
#include "../position/position.hpp"

struct SearchEntry;

template <bool silent>
void thread_search(int thread_idx, int search_depth);

enum class WorkerTask { None, Search, Refresh, NewGame };

struct SearchThread
{
    AccumulatorStack accumulator_stack;
    std::vector<SearchEntry> search_stack{140};
    History history{};
    Position position;
    State root_state{};
    std::list<Move> principal_variation{};
    std::atomic<uint64_t> node_searched{0};
    int score{negative_infinity};
    int nmp_min_ply{0};

    int seldepth{};
    int root_depth{};
    int id{};

    void search_stack_init();
    SearchThread();
    void new_game();
    void clear_tt() const;
};

struct ThreadPool
{
    static inline std::deque<SearchThread> threads{1};
    static inline std::deque<State> states{};

    static inline std::vector<std::jthread> os_threads;
    static inline std::mutex mtx;
    static inline std::condition_variable cv_start;
    static inline std::condition_variable cv_end;

    static inline std::atomic<bool> exit_flag{false};
    static inline std::atomic<int> active_workers{0};

    static inline auto current_task{WorkerTask::None};
    static inline uint32_t work_generation{0};
    static inline int current_search_depth{MAX_PLY};

    static void resize();

    static void start_workers(WorkerTask task);
    static void wait_for_workers();
    static void worker_loop(int thread_idx, uint32_t current_generation);
    static SearchThread& get(size_t index);
    static void setup();
    static void prepare();
    static uint64_t node_searched();
    static void shutdown();
};