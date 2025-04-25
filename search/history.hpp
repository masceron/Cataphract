#pragma once

#include <cstdint>
#include <forward_list>
#include <algorithm>
#include <array>
#include <cstring>

namespace History
{
    inline std::array<std::array<std::array<int16_t, 64>, 64>, 2> table;

    static constexpr int16_t min_bonus = -30000;
    static constexpr int16_t max_bonus = 30000;

    inline void clear()
    {
        for (int side = 0; side < 2; side++)
            for (int from = 0; from < 64; from++)
                memset(table[side][from].data(), 0, 64 * sizeof(int16_t));
    }

    inline void scale_down()
    {
        for (int side = 0; side < 2; side++)
            for (int from = 0; from < 64; from++) {
                std::ranges::transform(table[side][from].begin(), table[side][from].end(), table[side][from].begin(), [](const int16_t& x) {return x >> 1;});
            }
    }

    inline void apply(const bool side, const uint8_t from, const uint8_t to, const int16_t bonus)
    {
        const int16_t clamped_bonus = std::clamp(bonus, min_bonus, max_bonus);
        table[side][from][to] += clamped_bonus - table[side][from][to] * abs(clamped_bonus) / max_bonus;
    }

    inline void update(const std::forward_list<Move>& searched, const bool side, const uint8_t from, const uint8_t to, const uint8_t depth)
    {
        const int16_t bonus = 300 * depth - 250;
        apply(side, from, to, bonus);

        for (const Move& move: searched) {
            apply(side, move.src(), move.dest(), -bonus);
        }
        if (table[side][from][to] >= max_bonus) scale_down();
    }
}

namespace Killers
{
    inline std::array<std::array<Move, 2>, 127> table;
    inline void clear()
    {
        memset(table.data(), 0, table.size() * 2 * sizeof(Move));
    }
    inline void insert(const Move move, const uint8_t ply)
    {
        auto killers_of_ply = table[ply];
        if (move == killers_of_ply[0] || move == killers_of_ply[1]) return;
        killers_of_ply[1] = killers_of_ply[0];
        killers_of_ply[0] = move;
    }
    inline bool find(const Move move, const uint8_t ply)
    {
        return table[ply][0] == move || table[ply][1] == move;
    }
}

