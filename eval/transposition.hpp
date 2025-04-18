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
    exact, lower_bound, upper_bound
};

inline constexpr uint8_t age_delta = 4;

struct Entry
{
    uint16_t key;
    Move best_move;
    int8_t depth;
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
        return key & (table_size - 1);
    }

    inline std::pair<Bucket*, Entry*> probe(const uint64_t key, bool& ok, const int8_t depth, const uint8_t ply, const int16_t alpha, const int16_t beta, int16_t& score)
    {
        Bucket* bucket =  &(*table)[index_of(key)];
        const auto entry_key = static_cast<uint16_t>(key);
        Entry* _entry = nullptr;
        for (auto& entry : bucket->entries) {
            if (entry.key == entry_key) {
                if (entry.depth >= depth) {
                    score = entry.score;
                    if (score < -mate_bound) {
                        score += ply;
                    }
                    else if (score > mate_bound) {
                        score -= ply;
                    }
                    switch (entry.age_type & 0b11) {
                        case exact:
                            if (alpha + 1 == beta) {
                                ok = true;
                                _entry = &entry;
                            }
                            break;
                        default:
                            break;
                    }
                }
                break;
            }
        }
        return {bucket, _entry};
    }

    inline void write(Bucket* bucket, Entry* entry, const uint64_t key, const Move best_move, const int8_t depth, const uint8_t ply, int16_t score, const NodeType type)
    {
        if (score < -mate_bound) score -= ply;
        else if (score > mate_bound) score += ply;

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