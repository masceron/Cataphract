#pragma once

#include <cstdint>
#include <cstring>
#include <tuple>

#include "../options.hpp"
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
    inline static uintptr_t table_size = (64ull << 20) / sizeof(Entry);
    inline static uint8_t current_generation = 4;
    inline static Entry* table = nullptr;

    inline void clear()
    {
        current_generation = 4;
        std::memset(&table[0], 0, table_size * sizeof(Entry));
    }

    inline void free_tt()
    {
#ifdef _WIN32
        _aligned_free(table);
#else
        std::free(table);
#endif
    }

    inline void alloc()
    {
        Entry* new_table;
#ifdef _WIN32
        new_table = static_cast<Entry*>(_aligned_malloc(table_size * sizeof(Entry), sizeof(Entry)));
#else
        new_table = static_cast<Entry*>(std::aligned_alloc(sizeof(Entry), table_size * sizeof(Entry)));
#endif
        if (!new_table)
        {
            std::println("Cannot allocate new table.");

            if (table == nullptr) {
                std::exit(1);
            }
            return;
        }
        free_tt();
        table = new_table;
        clear();
    }

    inline void resize(const uint32_t new_size_in_mb)
    {
        if (new_size_in_mb != Options::hash)
        {
            Options::hash = new_size_in_mb;
            table_size = (static_cast<uint64_t>(new_size_in_mb) << 20) / sizeof(Entry);
            alloc();
        }
    }

    inline void advance()
    {
        current_generation += 4;
    }

    inline uint64_t index_of(const uint64_t& key)
    {
        return static_cast<uint64_t>((static_cast<__uint128_t>(key) * static_cast<__uint128_t>(table_size)) >> 64);
    }

    __attribute__((no_sanitize_thread))
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

    __attribute__((no_sanitize_thread))
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
            || key != entry->key
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
