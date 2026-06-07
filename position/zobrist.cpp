#include <random>
#include <algorithm>

#include "zobrist.hpp"

namespace Zobrist
{
    void generate_keys()
    {
        std::mt19937_64 gnr(541);
        std::uniform_int_distribution<uint64_t> dis;
        auto gen = [&]
        {
            return dis(gnr);
        };

        std::ranges::generate(castling_keys.begin(), castling_keys.end(), gen);
        for (auto& i : piece_keys)
        {
            std::ranges::generate(i.begin(), i.end(), gen);
        }

        std::ranges::generate(en_passant_key.begin(), en_passant_key.end(), gen);
        side_key = dis(gnr);
    }
}
