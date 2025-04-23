#pragma once

#include <cstdint>
#include <cstring>
#include "../position/move.hpp"

enum Values: int16_t
{
    draw = 0,
    infinity = 32000,
    negative_infinity = -32000,
    mate_value = 31999,
    mate_bound = mate_value - 128,
};

enum NodeType: uint8_t
{
    exact, lower_bound, upper_bound
};

struct Entry
{
    uint64_t key;
    Move best_move;
    int8_t depth;
    int16_t score;
    uint8_t type;
};

static constexpr uint64_t table_size = 16777216;

namespace TT
{
    inline auto table = new std::array<Entry, table_size>();

    inline uint64_t index_of(const uint64_t& key)
    {
        return key & (table_size - 1);
    }

    inline Entry* probe(const uint64_t key, bool& ok, const int8_t depth, const uint8_t ply, const int16_t alpha, const int16_t beta, int16_t& score)
    {
        Entry* entry = &(*table)[index_of(key)];
        if (entry->key == key) {
            if (ply > 0 && entry->depth >= depth && beta - alpha == 1) {
                score = entry->score;

                if (score < -mate_bound) score += ply;
                else if (score > mate_bound) score -= ply;

                if (entry->type == exact) {
                    ok = true;
                }
                else if (entry->type == lower_bound && score >= beta) {
                    score = beta;
                    ok = true;
                }
                else if (entry->type == upper_bound && score <= alpha) {
                    score = alpha;
                    ok = true;
                }
            }
        }
        return entry;
    }

    inline void write(Entry* entry, const uint64_t key, const Move best_move, const int8_t depth, const uint8_t ply, int16_t score, const NodeType type)
    {
        if (score < -mate_bound) score -= ply;
        else if (score > mate_bound) score += ply;

        *entry = Entry(key, best_move, depth, score, type);
    }

    inline void clear()
    {
        memset(&table[0], 0, table_size * sizeof(Entry));
    }
}