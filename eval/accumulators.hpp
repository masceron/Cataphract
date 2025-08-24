#pragma once

#include "nnue.hpp"

inline std::vector<Accumulator_entry> accumulator_stack;

inline constexpr uint8_t input_buckets_map[] = {
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 4, 5, 5, 5, 5, 4, 4,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9,
};

struct Finny_entry
{
    uint64_t bitboards[12];
    alignas(32) int16_t accumulators[2 * hl_size];
};

inline Finny_entry finny_table[input_buckets][input_buckets];

inline uint8_t flip_color(const uint8_t piece)
{
    switch (piece) {
        case P:
        case N:
        case B:
        case R:
        case Q:
        case K:
            return piece + 6;
        case p:
        case n:
        case b:
        case r:
        case q:
        case k:
            return piece - 6;
        default:
            return piece;
    }
}

inline uint64_t horizontal_mirror(uint64_t board)
{
    static constexpr uint64_t k1 = 0x5555555555555555ull;
    static constexpr uint64_t k2 = 0x3333333333333333ull;
    static constexpr uint64_t k4 = 0x0f0f0f0f0f0f0f0full;
    board = ((board >> 1) & k1) | ((board & k1) << 1);
    board = ((board >> 2) & k2) | ((board & k2) << 2);
    board = ((board >> 4) & k4) | ((board & k4) << 4);
    return board;
}

inline std::pair<uint16_t, uint16_t> input_index_of(const uint8_t p, const uint8_t sq,
                                                    const std::pair<bool, bool> &mirrors)
{
    const uint8_t white_flip = mirrors.first ? 7 : 0;
    const uint8_t black_flip = mirrors.second ? 7 : 0;

    return {p * 64 + (sq ^ 56 ^ white_flip), flip_color(p) * 64 + (sq ^ black_flip)};
}

inline std::pair<uint8_t, uint8_t> get_buckets(const uint64_t *boards)
{
    return {
        input_buckets_map[least_significant_one(boards[K]) ^ 56], input_buckets_map[least_significant_one(boards[k])]
    };
}

inline void accumulators_set(Network *network, const uint64_t *boards, int16_t *accumulators)
{
    memcpy(accumulators, network->accumulator_biases, hl_size * sizeof(int16_t));
    memcpy(&accumulators[hl_size], network->accumulator_biases, hl_size * sizeof(int16_t));

    const std::pair mirror = {least_significant_one(boards[K]) % 8 > 3, least_significant_one(boards[k]) % 8 > 3};
    const auto [white_bucket, black_bucket] = get_buckets(boards);

    for (int i = 0; i < 12; i++) {
        uint64_t board = boards[i];

        while (board) {
            auto [white_add, black_add] = input_index_of(i, least_significant_one(board), mirror);
            for (int iter = 0; iter < hl_size; iter++) {
                accumulators[iter] += network->accumulator_weights[white_bucket][white_add][iter];
                accumulators[iter + hl_size] += network->accumulator_weights[black_bucket][black_add][iter];
            }

            board &= board - 1;
        }
    }
}

inline void accumulators_addsub(Network *network, const int16_t *prev, int16_t *accs,
                                const std::pair<uint8_t, int8_t> *add,
                                const std::pair<uint8_t, int8_t> *sub, const std::pair<bool, bool> &is_mirrored,
                                const std::pair<uint8_t, uint8_t> &buckets)
{
    auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, is_mirrored);
    auto [white_sub, black_sub] = input_index_of(sub[0].first, sub[0].second, is_mirrored);

    for (int iter = 0; iter < hl_size; iter++) {
        accs[iter] = prev[iter] + network->accumulator_weights[buckets.first][white_add][iter]
                     - network->accumulator_weights[buckets.first][white_sub][iter];
        accs[iter + hl_size] = prev[iter + hl_size]
                               + network->accumulator_weights[buckets.second][black_add][iter]
                               - network->accumulator_weights[buckets.second][black_sub][iter];
    }
}

inline void accumulators_addsub2(Network *network, const int16_t *prev, int16_t *accs,
                                 const std::pair<uint8_t, int8_t> *add,
                                 const std::pair<uint8_t, int8_t> *sub, const std::pair<bool, bool> &is_mirrored,
                                 const std::pair<uint8_t, uint8_t> &buckets)
{
    auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, is_mirrored);
    auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, is_mirrored);
    auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, is_mirrored);

    for (uint16_t iter = 0; iter < hl_size; iter++) {
        accs[iter] = prev[iter]
                     + network->accumulator_weights[buckets.first][white_add][iter]
                     - network->accumulator_weights[buckets.first][white_sub1][iter]
                     - network->accumulator_weights[buckets.first][white_sub2][iter];

        accs[iter + hl_size] = prev[iter + hl_size]
                               + network->accumulator_weights[buckets.second][black_add][iter]
                               - network->accumulator_weights[buckets.second][black_sub1][iter]
                               - network->accumulator_weights[buckets.second][black_sub2][iter];
    }
}


inline void accumulators_add2sub2(Network *network, const int16_t *prev, int16_t *accs,
                                  const std::pair<uint8_t, int8_t> *add,
                                  const std::pair<uint8_t, int8_t> *sub, const std::pair<bool, bool> &is_mirrored,
                                  const std::pair<uint8_t, uint8_t> &buckets)
{
    auto [white_add1, black_add1] = input_index_of(add[0].first, add[0].second, is_mirrored);
    auto [white_add2, black_add2] = input_index_of(add[1].first, add[1].second, is_mirrored);
    auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, is_mirrored);
    auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, is_mirrored);

    for (int iter = 0; iter < hl_size; iter++) {
        accs[iter] = prev[iter]
                     + network->accumulator_weights[buckets.first][white_add1][iter]
                     + network->accumulator_weights[buckets.first][white_add2][iter]
                     - network->accumulator_weights[buckets.first][white_sub1][iter]
                     - network->accumulator_weights[buckets.first][white_sub2][iter];

        accs[iter + hl_size] = prev[iter + hl_size]
                               + network->accumulator_weights[buckets.second][black_add1][iter]
                               + network->accumulator_weights[buckets.second][black_add2][iter]
                               - network->accumulator_weights[buckets.second][black_sub1][iter]
                               - network->accumulator_weights[buckets.second][black_sub2][iter];
    }
}

inline void accumulator_stack_update(Network *network)
{
    auto idx = accumulator_stack.size() - 1;
    while (accumulator_stack[idx - 1].is_dirty) idx--;

    for (; idx < accumulator_stack.size(); idx++) {
        const auto stack_entry = &accumulator_stack[idx];
        const auto new_bitboards = stack_entry->bitboards;
        const auto [white_bucket, black_bucket] = get_buckets(new_bitboards);
        const std::pair new_mirrors = {
            least_significant_one(new_bitboards[K]) % 8 > 3, least_significant_one(new_bitboards[k]) % 8 > 3
        };
        const auto current_accumulators = stack_entry->accumulators;

        if (!stack_entry->require_rebuild) {
            const auto previous_accumulators = accumulator_stack[idx - 1].accumulators;
            if (stack_entry->subs[1].second != -1) {
                //Castling
                if (stack_entry->adds[1].second != -1) {
                    accumulators_add2sub2(network, previous_accumulators, current_accumulators,
                                          stack_entry->adds, stack_entry->subs, new_mirrors,
                                          {white_bucket, black_bucket});
                }
                //Capture or promo-capture
                else {
                    accumulators_addsub2(network, previous_accumulators, current_accumulators, stack_entry->adds,
                                         stack_entry->subs, new_mirrors,
                                         {white_bucket, black_bucket});
                }
            } else {
                accumulators_addsub(network, previous_accumulators, current_accumulators, stack_entry->adds,
                                    stack_entry->subs, new_mirrors, {white_bucket, black_bucket});
            }
        } else {
            const auto saved_entry = &finny_table[white_bucket][black_bucket];
            const auto saved_bitboards = saved_entry->bitboards;
            const std::pair old_mirrors = {
                saved_bitboards[K] && least_significant_one(saved_bitboards[K]) % 8 > 3,
                saved_bitboards[k] && least_significant_one(saved_bitboards[k]) % 8 > 3
            };
            const auto saved_accumulators = saved_entry->accumulators;

            for (int piece = 0; piece < 12; piece++) {
                const auto old_white = old_mirrors.first
                                           ? horizontal_mirror(saved_bitboards[piece])
                                           : saved_bitboards[piece];
                const auto old_black = old_mirrors.second
                                           ? horizontal_mirror(saved_bitboards[piece])
                                           : saved_bitboards[piece];
                const auto new_white = new_mirrors.first
                                           ? horizontal_mirror(new_bitboards[piece])
                                           : new_bitboards[piece];
                const auto new_black = new_mirrors.second
                                           ? horizontal_mirror(new_bitboards[piece])
                                           : new_bitboards[piece];

                auto white_adds = new_white & ~old_white;
                auto black_adds = new_black & ~old_black;
                while (white_adds) {
                    const int index_added = least_significant_one(white_adds);
                    const int input_index = (index_added ^ 56) + piece * 64;
                    for (int iter = 0; iter < hl_size; iter++) {
                        saved_accumulators[iter] += network->accumulator_weights[white_bucket][input_index][iter];
                    }
                    white_adds &= white_adds - 1;
                }

                while (black_adds) {
                    const int index_added = least_significant_one(black_adds);
                    const int input_index = index_added + flip_color(piece) * 64;
                    for (int iter = 0; iter < hl_size; iter++) {
                        saved_accumulators[iter + hl_size] += network->accumulator_weights[black_bucket][input_index][
                            iter];
                    }
                    black_adds &= black_adds - 1;
                }

                auto white_subs = old_white & ~new_white;
                auto black_subs = old_black & ~new_black;
                while (white_subs) {
                    const int index_subbed = least_significant_one(white_subs);
                    const int input_index = (index_subbed ^ 56) + piece * 64;
                    for (int iter = 0; iter < hl_size; iter++) {
                        saved_accumulators[iter] -= network->accumulator_weights[white_bucket][input_index][iter];
                    }
                    white_subs &= white_subs - 1;
                }

                while (black_subs) {
                    const int index_subbed = least_significant_one(black_subs);
                    const int input_index = index_subbed + flip_color(piece) * 64;
                    for (int iter = 0; iter < hl_size; iter++) {
                        saved_accumulators[iter + hl_size] -= network->accumulator_weights[black_bucket][input_index][
                            iter];
                    }
                    black_subs &= black_subs - 1;
                }
            }
            memcpy(saved_bitboards, new_bitboards, 12 * sizeof(uint64_t));
            memcpy(current_accumulators, saved_accumulators, 2 * hl_size * sizeof(int16_t));
        }

        stack_entry->is_dirty = false;
    }
}
