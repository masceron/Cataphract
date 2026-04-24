#pragma once

#include "movegen.hpp"
#include <chrono>
#include <print>

inline size_t perft(const int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    MoveList moves;
    legals<all>(position, moves);
    const auto n = moves.size();
    uint64_t nodes = 0;

    if (depth == 1)
    {
        return n;
    }

    State st;
    for (int i = 0; i < n; i++)
    {
        position.make_move(moves[i], st);
        nodes += perft(depth - 1);
        position.unmake_move(moves[i]);
    }

    return nodes;
}

inline void divide(const int depth)
{
    size_t total = 0;

    const auto start = std::chrono::high_resolution_clock::now();
    MoveList moves;
    legals<all>(position, moves);
    const auto n = moves.size();

    State st;

    for (int i = 0; i < n; i++)
    {
        position.make_move(moves[i], st);
        const size_t num = perft(depth - 1);
        std::println("{}: {}", moves[i].get_move_string(), num);
        std::fflush(stdout);
        total += num;
        position.unmake_move(moves[i]);
    }
    const auto time_taken = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start);
    std::println("Nodes searched: {}", total);
    std::println("Time taken: {:.2f}s",
                 static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(time_taken).count()) / 1000.0);
    std::println("Average: {:.2f} nodes per second.",
                 static_cast<double>(total) /
                 (static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count()) / 1000000000.0));
    std::fflush(stdout);
}
