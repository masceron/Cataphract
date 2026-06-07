#include <numeric>

#include "thread.hpp"
#include "../eval/transposition.hpp"
#include "../eval/nnue.hpp"

void SearchThread::search_stack_init()
{
    for (unsigned long long i = 0; i < search_stack.size(); i++)
    {
        search_stack[i].plies = static_cast<uint8_t>(i - 4);
    }
}

SearchThread::SearchThread()
{
    history.clear();
}

void SearchThread::new_game()
{
    history.clear();
    clear_tt();
}

void SearchThread::clear_tt() const
{
    const auto length = TT::table_size / Options::threads;
    const auto start = length * id;
    const auto clear_len = id == Options::threads - 1 ? TT::table_size - start : length;

    TT::clear(&TT::table[start], clear_len);
}

void ThreadPool::resize()
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

void ThreadPool::start_workers(const WorkerTask task)
{
    std::unique_lock lock(mtx);
    current_task = task;
    active_workers = static_cast<int>(threads.size()) - 1;
    work_generation++;
    cv_start.notify_all();
}

void ThreadPool::wait_for_workers()
{
    std::unique_lock lock(mtx);
    cv_end.wait(lock, [] { return active_workers.load() == 0; });
    current_task = WorkerTask::None;
}

void ThreadPool::worker_loop(const int thread_idx, uint32_t current_generation)
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
            refresh_accumulators(thread.position, thread.accumulator_stack);
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

SearchThread& ThreadPool::get(const size_t index)
{
    return threads[index];
}

void ThreadPool::setup()
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

void ThreadPool::prepare()
{
    for (auto& thread : threads)
    {
        thread.node_searched = 0;
        thread.seldepth = 0;
        thread.principal_variation.clear();
        thread.score = negative_infinity;
        thread.nmp_min_ply = 0;
    }
}

uint64_t ThreadPool::node_searched()
{
    return std::accumulate(threads.begin(), threads.end(), 0ull, [](const uint64_t sum, const SearchThread& thread)
    {
        return sum + thread.node_searched.load(std::memory_order_relaxed);
    });
}

void ThreadPool::shutdown()
{
    if (!os_threads.empty())
    {
        exit_flag = true;
        cv_start.notify_all();
        os_threads.clear();
    }
}