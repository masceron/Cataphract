#pragma once

#include <cstdint>
#include "move.hpp"

enum Values: int16_t
{
    draw = 0,
    negative_infinity = -32000,
    infinity = -negative_infinity,
    mate_value = infinity - 1,
    mate_bound = mate_value - 256,
};

enum NodeType: uint8_t
{
    pv_node, all_node, cut_node
};

inline constexpr uint8_t age_delta = 4;

struct Entry
{
    uint16_t key;
    Move best_move;
    uint8_t depth;
    int16_t score;
    uint8_t age_type;
};

struct Bucket
{
    Entry entries[3];
};

static constexpr uint64_t table_size = 8388608;

namespace TT
{
    inline auto table = new std::array<Bucket, table_size>();
    inline uint8_t age = 0;

    inline uint64_t index_of(const uint64_t& key)
    {
        static constexpr uint64_t multiplier = 0x9e3779b97f4a7c15ull;
        return (key * multiplier) & (table_size - 1);
    }

    inline std::pair<Bucket*, Entry*> probe(const uint64_t key, bool& ok)
    {
        const uint64_t index = index_of(key);
        const auto entry_key = static_cast<uint16_t>(key);

        for (auto& entry : (*table)[index].entries) {
            if (entry.key == entry_key) {
                return {&(*table)[index], &entry};
            }
        }
        ok = false;
        return {&(*table)[index], nullptr};
    }

    inline void write(Bucket* bucket, Entry* entry, const uint64_t key, const Move best_move, const uint8_t depth, int16_t score, const NodeType type)
    {
        if (score < -mate_bound) score -= depth;
        else if (score > mate_bound) score += depth;

        const auto entry_key = static_cast<uint16_t>(key);
        if (entry != nullptr) {
            *entry = Entry(entry_key, best_move, depth, score, type + age);
            return;
        }

        uint8_t shallowest = 0;

        for (int8_t i = 0; i < 3; i++) {
            if ((bucket->entries[i].age_type & 252) != age) {
                bucket->entries[i] = Entry(entry_key, best_move, depth, score, type + age);
                return;
            }
            if (bucket->entries[i].depth < bucket->entries[shallowest].depth) shallowest = i;
        }

        bucket->entries[shallowest] = Entry(entry_key, best_move, depth, score, type + age);
    }

    inline void clear()
    {
        age = 0;
        memset(&table[0], 0, table_size * sizeof(Bucket));
    }
}