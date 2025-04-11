#pragma once

#include "movegen.hpp"
#include <chrono>
#include <iomanip>

inline size_t perft(const int depth)
{
    if (depth == 0) {
        return 1;
    }

    MoveList moves;
    legals<captures>(position, moves);
    legals<quiet>(position, moves);
    const int n = moves.size();
    uint64_t nodes = 0;

    if (depth == 1) {
        return n;
    }

    State st;
    for (int i = 0; i < n; i++) {
        position.make_move(moves.list[i], st);
        nodes += perft(depth - 1);
        position.unmake_move(moves.list[i]);
    }

    return nodes;
}

inline void divide(const int depth)
{
    size_t total = 0;
    MoveList list;

    const auto start = std::chrono::high_resolution_clock::now();
    legals<all>(position, list);
    const int n = list.size();

    State st;

    for (int i = 0; i < n; i++) {
        position.make_move(list.list[i], st);
        const size_t num = perft(depth - 1);
        std::cout << get_move_string(list.list[i]) << ": " << num << "\n";
        total += num;
        position.unmake_move(list.list[i]);
    }
    const auto time_taken = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start);
    std::cout << "\nNodes searched: " << total << "\n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_taken) << "\n";
    std::cout << "Average: " << std::fixed << std::setprecision(2)
              << total / (std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count() /1000000000.0)
              << " nodes per second.\n\n";
}