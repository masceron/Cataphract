#pragma once

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "move.hpp"

enum Values
{
    draw = 0,
    negative_infinity = -32000,
    infinity = -negative_infinity,
    mate_value = infinity - 1,
    mate_bound = mate_value - 256,
};

enum NodeType: uint8_t
{
    pv_node = 1, all_node, cut_node
};

struct Entry
{
    uint64_t key;
    Move best_move;
    uint8_t depth;
    int16_t score;
    NodeType type;
};

constexpr uint16_t table_size_in_mb = 128;

namespace Table
{
    inline auto table = new std::array<Entry, table_size_in_mb * (1024 * 1024) / sizeof(Entry)>();
    inline uint64_t size = table->size();

    inline uint64_t index_of(const uint64_t& key)
    {
        uint64_t index;
    #ifdef __GNUC__
        const __uint128_t prod = static_cast<__uint128_t>(key) * size;
        index = prod >> 64;
    #elif _MSC_VER
        index = __umulh(key, size);
    #endif
        return index;
    }

    inline Entry* probe(const uint64_t key, bool& ok)
    {
        const uint64_t index = index_of(key);

        if ((*table)[index].key != key) {
            ok = false;
        }
        return &(*table)[index];
    }

    inline void write(Entry* entry, const uint64_t key, const Move best_move, const uint8_t depth, int16_t score, const NodeType type)
    {
        if (score < -mate_bound) score -= depth;
        else if (score > mate_bound) score += depth;

        *entry = Entry(key, best_move, depth, score, type);
    }

    inline void clear()
    {
        std::memset(&(*table)[0], 0, table->size() * sizeof(Entry));
    }
}