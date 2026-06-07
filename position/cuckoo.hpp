#pragma once
#include <vector>

#include "move.hpp"

namespace Cuckoo
{
    inline std::vector<uint64_t> cuckoo_key(8192, 0);
    inline std::vector cuckoo_move(8192, null_move);

    uint64_t hash1(uint64_t key);
    uint64_t hash2(uint64_t key);

    void init();
}