#pragma once

#include <array>

namespace Zobrist
{
    inline std::array<std::array<uint64_t, 64>, 14> piece_keys;
    inline std::array<uint64_t, 16> castling_keys;
    inline std::array<uint64_t, 8> en_passant_key;
    inline uint64_t side_key;

    void generate_keys();
}