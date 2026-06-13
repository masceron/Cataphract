#include <array>

#include "arch.hpp"
#include "../board/bitboard.hpp"
#include "utils.hpp"

uint8_t flip_color(const uint8_t piece)
{
    return piece ^ 8;
}

int nnue_index(const uint8_t piece) {
    return piece - ((piece & 8) >> 2);
}

std::pair<int, int> input_index_of(const uint8_t piece, const uint8_t square,
                                                    const std::pair<bool, bool>& mirrors)
{
    return {
        nnue_index(piece) * 64 + (square ^ 56 ^ (mirrors.first ? 7 : 0)),
        nnue_index(flip_color(piece)) * 64 + (square ^ (mirrors.second ? 7 : 0))
    };
}

uint64_t horizontal_mirror(uint64_t board)
{
    static constexpr uint64_t k1 = 0x5555555555555555ull;
    static constexpr uint64_t k2 = 0x3333333333333333ull;
    static constexpr uint64_t k4 = 0x0f0f0f0f0f0f0f0full;
    board = ((board >> 1) & k1) | ((board & k1) << 1);
    board = ((board >> 2) & k2) | ((board & k2) << 2);
    board = ((board >> 4) & k4) | ((board & k4) << 4);
    return board;
}

std::pair<uint8_t, uint8_t> get_buckets(const std::array<uint64_t, 14>& boards)
{
    return {
        input_buckets_map[lsb(boards[K]) ^ 56], input_buckets_map[lsb(boards[k])]
    };
}