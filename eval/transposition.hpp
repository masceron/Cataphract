#pragma once

#include <cstdint>
#include <cstring>
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
    uint8_t depth;
    int16_t score;
    int16_t static_eval;
    Move best_move;
    uint8_t age_type;

    void save(const uint64_t _key, const uint8_t _depth, const int16_t _static_eval, const int16_t _score , const Move _best_move, const uint8_t _age_type)
    {
        key = _key;
        depth = _depth;
        score = _score;
        static_eval = _static_eval;
        best_move = _best_move;
        age_type = _age_type;
    }
};
#pragma pack(pop)

namespace TT
{
    inline uint32_t table_size_in_mb = 256;
    inline uint32_t table_size = table_size_in_mb * 1048576 / sizeof(Entry);
    inline uint8_t current_age = 4;
    inline Entry* table;

    inline void clear()
    {
        current_age = 4;
        memset(&table[0], 0, table_size * sizeof(Entry));
    }

    inline void alloc()
    {
#ifdef _WIN32
        table = static_cast<Entry*>(_aligned_malloc(table_size * 16, 16));
#else
        table = static_cast<Entry*>(std::aligned_alloc(16, table_size * 16));
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
        if (new_size_in_mb != table_size_in_mb) {
            free_tt();
            table_size_in_mb = new_size_in_mb;
            table_size = table_size_in_mb * 1048576 / 16;
            alloc();
        }
    }

    inline void age()
    {
        current_age += 4;
    }

    inline uint64_t index_of(const uint64_t& key)
    {
        return (static_cast<unsigned __int128>(key) * static_cast<unsigned __int128>(table_size)) >> 64;
    }

    inline std::tuple<Entry*, uint8_t, uint8_t, Move, int16_t> probe(const uint64_t key, bool& match, const uint8_t ply, int& score)
    {
        Entry* entry = &table[index_of(key)];
        if (entry->key == key) {
            match = true;
            score = static_cast<int>(entry->score);
            if (score < mated_in_max_ply)
                score += ply;
            else if (score > mate_in_max_ply)
                score -= ply;
        }
        return {entry, entry->depth, entry->age_type, entry->best_move, entry->static_eval};
    }

    inline void write(Entry* entry, const uint64_t key, const Move best_move, const int depth, const uint8_t ply, const int static_eval, int score, const NodeType type)
    {
        if (score < mated_in_max_ply) {
            score -= ply;
        }
        else if (score > mate_in_max_ply) {
            score += ply;
        }

        entry->save(key, static_cast<uint8_t>(depth), static_cast<int16_t>(static_eval), static_cast<int16_t>(score), best_move, current_age + type);
    }

    inline uint16_t full()
    {
        uint16_t hashfull = 0;
        for (int i = 0; i < 1000; i++) {
            if ((table[i].age_type & 0b11111100) == current_age) hashfull++;
        }
        return hashfull;
    }
}