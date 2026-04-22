#pragma once

#include <cstdint>
#include <cstring>
#include <bit>
#include <tuple>
#include "../position/move.hpp"

enum Values: int
{
    draw = 0,
    infinity = 32000,
    negative_infinity = -32000,
    mate_value = 31999,
    mate_in_max_ply = mate_value - 128,
    mated_in_max_ply = -mate_in_max_ply,
    score_none = -32500
};

enum NodeType: uint8_t
{
    exact, lower_bound, upper_bound
};

#pragma pack(push, 1)
struct Entry
{
    uint64_t key;
    int16_t score;
    int16_t static_eval;
    Move best_move;
    uint8_t depth;
    uint8_t age_type;
};
#pragma pack(pop)

namespace TT
{
    static uint64_t closest_2_pow(const uint32_t mb)
    {
        [[assume(mb != 0)]];
        const uint64_t total_bytes = static_cast<uint64_t>(mb) << 20;
        return std::bit_floor(total_bytes / sizeof(Entry));
    }

    inline static uint32_t table_size_in_mb = 256;
    inline static uint64_t table_size = closest_2_pow(table_size_in_mb);
    inline static uint64_t hash_mask = table_size - 1;
    inline static uint8_t current_generation = 4;
    inline static Entry* table;

    inline void clear()
    {
        current_generation = 4;
        memset(&table[0], 0, table_size * sizeof(Entry));
    }

    inline void alloc()
    {
#ifdef _WIN32
        table = static_cast<Entry*>(_aligned_malloc(table_size * sizeof(Entry), sizeof(Entry)));
#else
        table = static_cast<Entry*>(std::aligned_alloc(sizeof(Entry), table_size * sizeof(Entry)));
#endif
        clear();
    }

    inline void free_tt()
    {
#ifdef _WIN32
        _aligned_free(table);
#else
        std::free(table);
#endif
    }

    inline void resize(const uint32_t new_size_in_mb)
    {
        if (new_size_in_mb != table_size_in_mb)
        {
            free_tt();
            table_size_in_mb = new_size_in_mb;
            table_size = closest_2_pow(new_size_in_mb);
            hash_mask = table_size - 1;
            alloc();
        }
    }

    inline void advance()
    {
        current_generation += 4;
    }

    inline uint64_t index_of(const uint64_t& key)
    {
        return key & hash_mask;
    }

    inline std::tuple<Entry*, int, uint8_t, Move, int> probe(const uint64_t key, bool& match, const uint8_t ply,
                                                             int& score)
    {
        Entry* entry = &table[index_of(key)];
        if (entry->key == key)
        {
            match = true;
            score = static_cast<int>(entry->score);
            if (score < mated_in_max_ply)
                score += ply;
            else if (score > mate_in_max_ply)
                score -= ply;
        }
        return {entry, entry->depth, entry->age_type, entry->best_move, entry->static_eval};
    }

    inline void write(Entry* entry, const uint64_t key, const Move best_move, const int depth, const uint8_t ply,
                      const int static_eval, int score, const NodeType type)
    {
        if (score < mated_in_max_ply)
        {
            score -= ply;
        }
        else if (score > mate_in_max_ply)
        {
            score += ply;
        }

        if (best_move || entry->key != key)
        {
            entry->best_move = best_move;
        }

        if (type == exact
            || (entry->age_type & 252) != current_generation
            || key !=entry->key
            || depth + 4 >= entry->depth)
        {
            entry->key = key;
            entry->depth = depth;
            entry->score = static_cast<int16_t>(score);
            entry->static_eval = static_cast<int16_t>(static_eval);
            entry->age_type = current_generation + type;
        }
    }

    inline uint16_t full()
    {
        uint16_t hash_full = 0;
        for (int i = 0; i < 1000; i++)
        {
            if ((table[i].age_type & 0b11111100) == current_generation) hash_full++;
        }
        return hash_full;
    }
}
