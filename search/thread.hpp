#pragma once

#include <vector>
#include <condition_variable>
#include <thread>
#include <list>

#include "../position/position.hpp"
#include "history.hpp"
#include "../options.hpp"
#include "../eval/nnue.hpp"

template <bool silent>
void thread_search(int thread_idx, int search_depth);

enum class WorkerTask { None, Search, Refresh, NewGame };

struct SearchThread
{
    std::vector<SearchEntry> search_stack{140};
    History history{};
    Position position;
    State root_state{};
    AccumulatorStack accumulator_stack;
    std::atomic<uint64_t> node_searched{0};
    std::list<Move> principal_variation{};
    int score{negative_infinity};

    int seldepth{};
    int root_depth{};
    int max_depth{};
    int id{};

    void search_stack_init()
    {
        for (unsigned long long i = 0; i < search_stack.size(); i++)
        {
            search_stack[i].plies = static_cast<uint8_t>(i - 4);
        }
    }

    SearchThread()
    {
        history.clear();
    }

    void new_game()
    {
        history.clear();
        clear_tt();
    }

    void clear_tt() const
    {
        const auto length = TT::table_size / Options::threads;
        const auto start = length * id;
        const auto clear_len = id == Options::threads - 1 ? TT::table_size - start : length;

        TT::clear(&TT::table[start], clear_len);
    }
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

    static void resize()
    {
        if (!os_threads.empty())
        {
            exit_flag = true;
            cv_start.notify_all();
            os_threads.clear();
            exit_flag = false;
        }

        threads.resize(Options::threads);
        for (int i = 0; i < Options::threads; i++) threads[i].id = i;

        setup();

        uint32_t starting_generation;
        {
            std::unique_lock lock(mtx);
            starting_generation = work_generation;
        }

        for (int i = 1; i < Options::threads; i++)
        {
            os_threads.emplace_back([i, starting_generation] { worker_loop(i, starting_generation); });
        }

        start_workers(WorkerTask::Refresh);
        wait_for_workers();
    }

    static void start_workers(const WorkerTask task)
    {
        std::unique_lock lock(mtx);
        current_task = task;
        active_workers = static_cast<int>(threads.size()) - 1;
        work_generation++;
        cv_start.notify_all();
    }

    static void wait_for_workers()
    {
        std::unique_lock lock(mtx);
        cv_end.wait(lock, [] { return active_workers.load() == 0; });
        current_task = WorkerTask::None;
    }

    static void worker_loop(const int thread_idx, uint32_t current_generation)
    {
        while (true)
        {
            std::unique_lock lock(mtx);
            cv_start.wait(lock, [&current_generation]
            {
                return work_generation != current_generation || exit_flag.load();
            });

            if (exit_flag.load()) return;

            current_generation = work_generation;
            const WorkerTask task = current_task;
            lock.unlock();

            auto& thread = threads[thread_idx];

            if (task == WorkerTask::Refresh)
            {
                NNUE::refresh_accumulators(thread.position, thread.accumulator_stack);
            }
            else if (task == WorkerTask::Search)
            {
                thread_search<true>(thread_idx, current_search_depth);
            }
            else if (task == WorkerTask::NewGame)
            {
                thread.new_game();
            }

            {
                std::unique_lock tmp_lock(mtx);
                --active_workers;
            }

            cv_end.notify_one();
        }
    }

    static SearchThread& get(const size_t index)
    {
        return threads[index];
    }

    static void setup()
    {
        const auto& master_thread = threads[0];
        for (size_t i = 1; i < threads.size(); i++)
        {
            auto& thread = threads[i];
            thread.position = master_thread.position;

            if (master_thread.position.state)
            {
                thread.root_state = *master_thread.position.state;
                thread.position.state = &thread.root_state;
            }
        }
    }

    static void prepare()
    {
        for (auto& thread : threads)
        {
            thread.node_searched = 0;
            thread.seldepth = 0;
            thread.principal_variation.clear();
            thread.score = negative_infinity;
        }
    }

    static uint64_t node_searched()
    {
        return std::accumulate(threads.begin(), threads.end(), 0ull, [](const uint64_t sum, const SearchThread& thread)
        {
            return sum + thread.node_searched.load(std::memory_order_relaxed);
        });
    }

    static void shutdown()
    {
        if (!os_threads.empty())
        {
            exit_flag = true;
            cv_start.notify_all();
            os_threads.clear();
        }
    }
};