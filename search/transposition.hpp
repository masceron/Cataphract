#pragma once

#include <cstdint>
#include <tuple>

#include "../position/move.hpp"

enum class NodeType: uint8_t
{
    none, exact, lower_bound, upper_bound
};

struct Entry
{
    uint64_t key;
    int16_t score;
    int16_t static_eval;
    Move best_move;
    uint8_t depth;
    uint8_t age_pv_type;

    [[nodiscard]] bool is_pv() const
    {
        return ((age_pv_type >> 2) & 1) != 0;
    }
};

namespace TT
{
    inline uintptr_t table_size = (64ull << 20) / sizeof(Entry);
    inline uint8_t current_generation = 4;
    inline Entry* table = nullptr;

    void clear(Entry* start, size_t length);
    void free_tt();
    void alloc();
    void resize(uint32_t new_size_in_mb);
    void advance();
    __attribute__((no_sanitize_thread))
    std::tuple<Entry*, int, NodeType, Move, int, int> probe(uint64_t key, bool& match, uint8_t ply);
    __attribute__((no_sanitize_thread))
    void write(Entry* entry, uint64_t key, Move best_move, int depth, uint8_t ply,
               int static_eval, int score, NodeType type, bool pv);

    uint16_t full();
}
