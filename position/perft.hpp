#pragma once

#include "movegen.hpp"
#include <chrono>
#include <iomanip>

inline size_t perft(const int depth)
{
    if (depth == 0) {
        return 1;
    }

    const MoveList moves = legals<all>(position);
    const int n = moves.size();
    uint64_t nodes = 0;

    if (depth == 1) {
        return n;
    }

    State st;
    for (int i = 0; i < n; i++) {
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
    const MoveList list = legals<all>(position);
    const int n = list.size();

    State st;

    for (int i = 0; i < n; i++) {
        position.make_move(list[i], st);
        const size_t num = perft(depth - 1);
        std::cout << list[i].get_move_string() << ": " << num << std::endl;
        total += num;
        position.unmake_move(list[i]);
    }
    const auto time_taken = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start);
    std::cout << "\nNodes searched: " << total << "\n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_taken) << "\n";
    std::cout << "Average: " << std::fixed << std::setprecision(2)
              << total / (std::chrono::duration_cast<std::chrono::nanoseconds>(time_taken).count() /1000000000.0)
              << " nodes per second.\n" << std::endl;
}