#include "params.hpp"

#include <array>
#include <cmath>

void reduction_cal()
{
    for (int depth = 0; depth < MAX_PLY; depth++)
    {
        for (int numMoves = 0; numMoves < 63; numMoves++)
        {
            reductions[depth][numMoves] = std::floor(
                lmr_base() + std::log(depth + 1) * std::log(numMoves + 1) / lmr_div());
        }
    }
}

void prune_cal()
{
    for (int i = 0; i < 16; i++)
    {
        lmp[0][i] = std::floor(lmp_base() + i * i / lmp_nidiv());
        lmp[1][i] = std::floor(lmp_base() + i * i / lmp_idiv());
    }
}
