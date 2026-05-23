#include "params.hpp"

#include <array>
#include <cmath>
#include <cstdint>

void reduction_cal()
{
    for (int depth = 0; depth < 127; depth++)
    {
        for (int numMoves = 0; numMoves < 63; numMoves++)
        {
            reductions[depth][numMoves] = static_cast<uint8_t>(std::floor(
                lmr_base() + std::log(depth + 1) * std::log(numMoves + 1) / lmr_div()));
        }
    }
}

void prune_cal()
{
    for (int i = 0; i < 16; i++)
    {
        lmp[0][i] = static_cast<uint8_t>(std::floor(lmp_base() + i * i / lmp_nidiv()));
        lmp[1][i] = static_cast<uint8_t>(std::floor(lmp_base() + i * i / lmp_idiv()));
    }
}