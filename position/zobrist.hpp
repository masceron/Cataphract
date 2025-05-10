#pragma once

#include <cstdint>
#include <random>
#include <array>
#include <algorithm>

namespace Zobrist
{
    inline std::array<std::array<uint64_t, 64>, 12> piece_keys;
    inline std::array<uint64_t, 16> castling_keys;
    inline std::array<uint64_t, 8> en_passant_key;
    inline uint64_t side_key;

    inline void generate_keys()
    {
        std::mt19937_64 gnr(541);
        std::uniform_int_distribution<uint64_t> dis;
        auto gen = [&] {return dis(gnr);};
        std::ranges::generate(castling_keys.begin(), castling_keys.end(), gen);
        for (auto& i: piece_keys) {
            std::ranges::generate(i.begin(), i.end(), gen);
        }
        std::ranges::generate(en_passant_key.begin(), en_passant_key.end(), gen);
        side_key = dis(gnr);
    }
}

