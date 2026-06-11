#include <print>
#include <utility>
#include <cstring>

#include "transposition.hpp"
#include "../options.hpp"

namespace TT
{
    void clear(Entry* start, const size_t length)
    {
        std::memset(start, 0, length * sizeof(Entry));
    }

    void free_tt()
    {
#ifdef _WIN32
        _aligned_free(table);
#else
        std::free(table);
#endif
    }

    void alloc()
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
        clear(table, table_size);
    }

    void resize(const uint32_t new_size_in_mb)
    {
        if (new_size_in_mb != Options::hash)
        {
            Options::hash = new_size_in_mb;
            table_size = (static_cast<uint64_t>(new_size_in_mb) << 20) / sizeof(Entry);
            alloc();
        }
    }

    void advance()
    {
        current_generation += 8;
    }

    uint64_t index_of(const uint64_t& key)
    {
        return static_cast<uint64_t>((static_cast<__uint128_t>(key) * static_cast<__uint128_t>(table_size)) >> 64);
    }

    __attribute__((no_sanitize_thread))
    std::tuple<Entry*, int, NodeType, Move, int, int> probe(const uint64_t key, bool& match, const uint8_t ply)
    {
        Entry* entry = &table[index_of(key)];
        int score;
        if (entry->key == key)
        {
            match = true;
            score = static_cast<int>(entry->score);
            if (score < mated_in_max_ply)
                score += ply;
            else if (score > mate_in_max_ply)
                score -= ply;
        }
        return {entry, entry->depth, static_cast<NodeType>(entry->age_pv_type & 0b11), entry->best_move, entry->static_eval, score};
    }

    __attribute__((no_sanitize_thread))
    void write(Entry* entry, const uint64_t key, const Move best_move, const int depth, const uint8_t ply,
                      const int static_eval, int score, const NodeType type, const bool pv)
    {
        if (score < mated_in_max_ply)
        {
            score -= ply;
        }
        else if (score > mate_in_max_ply)
        {
            score += ply;
        }

        if (!(type == NodeType::exact
            || (entry->age_pv_type & 0b11111000) != current_generation
            || key != entry->key
            || depth + 4 + entry->is_pv() * 2 >= entry->depth))
            return;

        if (best_move || entry->key != key)
        {
            entry->best_move = best_move;
        }

        entry->key = key;
        entry->depth = depth;
        entry->score = static_cast<int16_t>(score);
        entry->static_eval = static_cast<int16_t>(static_eval);
        entry->age_pv_type = current_generation + std::to_underlying(type) + (pv << 2);
    }

    uint16_t full()
    {
        uint16_t hash_full = 0;
        for (int i = 0; i < 1000; i++)
        {
            if ((table[i].age_pv_type & 0b11111000) == current_generation) hash_full++;
        }
        return hash_full;
    }
}
