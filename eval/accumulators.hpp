#pragma once

#include "utils.hpp"
#include "../position/position.hpp"

#include "simd/simd.hpp"

struct AccumulatorEntry
{
    SIMD_ALIGN int16_t accumulators[2 * HL_SIZE];
    uint64_t bitboards[12];
    std::pair<uint8_t, int8_t> adds[2];
    std::pair<uint8_t, int8_t> subs[2];
    bool is_dirty;
    bool require_rebuild;

    void mark_changes(const Position& pos, const Move move)
    {
        is_dirty = true;
        require_rebuild = false;

        adds[0] = {0, -1};
        adds[1] = {0, -1};
        subs[0] = {0, -1};
        subs[1] = {0, -1};

        const uint8_t from = move.from();
        const uint8_t to = move.to();
        const auto flag = move.flag();
        const Pieces moved_piece = pos.piece_on[to];
        std::memcpy(bitboards, &pos.boards, 12 * sizeof(uint64_t));
        const bool just_moved = !pos.side_to_move;

        if (moved_piece == K || moved_piece == k)
        {
            if (const int flip = !just_moved * 56;
                input_buckets_map[from ^ flip] != input_buckets_map[to ^ flip] || ((from % 8 > 3) != (to % 8 > 3)))
            {
                require_rebuild = true;
            }
        }

        if (flag < MoveFlag::knight_promotion)
        {
            adds[0] = {moved_piece, to};
            subs[0] = {moved_piece, from};
            if (flag == MoveFlag::capture)
            {
                subs[1] = {pos.state->captured_piece, to};
            }
            else if (flag == MoveFlag::ep_capture)
            {
                subs[1] = {pos.state->captured_piece, to - Delta<Direction::Up>(just_moved)};
            }
            else if (flag == MoveFlag::king_castle)
            {
                auto rook = just_moved == white ? R : r;
                adds[1] = {rook, to - 1};
                subs[1] = {rook, to + 1};
            }
            else if (flag == MoveFlag::queen_castle)
            {
                auto rook = just_moved == white ? R : r;
                adds[1] = {rook, to + 1};
                subs[1] = {rook, to - 2};
            }
        }
        else
        {
            subs[0] = {!just_moved ? P : p, from};
            adds[0] = {move.promoted_to(just_moved), to};
            if (flag >= MoveFlag::knight_promo_capture)
            {
                subs[1] = {pos.state->captured_piece, to};
            }
        }
    }
};

struct FinnyEntry
{
    uint64_t bitboards[12];
    SIMD_ALIGN int16_t accumulators[2 * HL_SIZE];
};

struct AccumulatorStack
{
    AccumulatorEntry stack[130];
    int size;
    FinnyEntry finny_table[INPUT_BUCKETS][INPUT_BUCKETS];

    AccumulatorStack() : stack{}, size(0)
    {
    }

    void push(const Position& pos, const Move move)
    {
        stack[size].mark_changes(pos, move);
        ++size;
    }

    void push()
    {
        ++size;
    }

    void clear()
    {
        size = 0;
    }

    void pop()
    {
        --size;
    }

    AccumulatorEntry* operator[](const int idx)
    {
        return &stack[idx];
    }
};

inline void accumulators_set(const Network* __restrict network, const uint64_t* __restrict boards, int16_t* __restrict accumulators)
{
    std::memcpy(accumulators, network->accumulator_biases, HL_SIZE * sizeof(int16_t));
    std::memcpy(&accumulators[HL_SIZE], network->accumulator_biases, HL_SIZE * sizeof(int16_t));

    const std::pair mirror = {lsb(boards[K]) % 8 > 3, lsb(boards[k]) % 8 > 3};
    const auto [white_bucket, black_bucket] = get_buckets(boards);

    for (int i = 0; i < 12; i++)
    {
        uint64_t board = boards[i];

        while (board)
        {
            auto [white_add, black_add] = input_index_of(i, pop_lsb(board), mirror);
            for (int iter = 0; iter < HL_SIZE; iter++)
            {
                accumulators[iter] = static_cast<int16_t>(accumulators[iter]
                    + network->accumulator_weights[white_bucket][white_add][iter]);
                accumulators[iter + HL_SIZE] = static_cast<int16_t>(accumulators[iter + HL_SIZE] + network->
                    accumulator_weights[black_bucket][black_add][iter]);
            }
        }
    }
}

template <const int exclude>
void update_from_move(Network* __restrict network, int16_t* __restrict prev, int16_t* __restrict cur,
                      const std::pair<uint8_t, int8_t>* add,
                      const std::pair<uint8_t, int8_t>* sub,
                      const std::pair<bool, bool>& mirrors,
                      const std::pair<uint8_t, uint8_t>& buckets)
{
    if (sub[1].second != -1)
    {
        //Castling
        if (add[1].second != -1)
        {
            SIMD::accumulators_add2sub2<exclude>(network, prev, cur,
                                           add, sub, mirrors,
                                           buckets);
        }
        //Capture or promo-capture
        else
        {
            SIMD::accumulators_addsub2<exclude>(network, prev, cur, add,
                                          sub, mirrors,
                                          buckets);
        }
    }
    else
    {
        SIMD::accumulators_addsub<exclude>(network, prev, cur, add,
                                     sub, mirrors,
                                     buckets);
    }
}

inline void accumulator_stack_update(Network* __restrict network, AccumulatorStack& accumulator_stack)
{
    auto idx = accumulator_stack.size - 1;
    while (accumulator_stack[idx - 1]->is_dirty) idx--;
    auto& finny_table = accumulator_stack.finny_table;

    for (; idx < accumulator_stack.size; idx++)
    {
        const auto stack_entry = accumulator_stack[idx];
        const auto new_bitboards = stack_entry->bitboards;
        const auto new_buckets = get_buckets(new_bitboards);
        const std::pair new_mirrors = {
            lsb(new_bitboards[K]) % 8 > 3, lsb(new_bitboards[k]) % 8 > 3
        };
        const auto current_accumulators = stack_entry->accumulators;

        const auto previous_entry = accumulator_stack[idx - 1];
        const auto previous_boards = previous_entry->bitboards;
        const auto previous_accumulators = accumulator_stack[idx - 1]->accumulators;

        if (!stack_entry->require_rebuild)
        {
            update_from_move<-1>(network, previous_accumulators, current_accumulators, stack_entry->adds,
                                 stack_entry->subs, new_mirrors, new_buckets);
        }
        else
        {
            const auto previous_buckets = get_buckets(previous_boards);
            const auto [white_bucket, black_bucket] = new_buckets;
            const auto saved_entry = &finny_table[white_bucket][black_bucket];
            const auto saved_bitboards = saved_entry->bitboards;
            const auto saved_accumulators = saved_entry->accumulators;

            bool to_update;

            const std::pair saved_mirrors = {
                saved_bitboards[K] && lsb(saved_bitboards[K]) % 8 > 3,
                saved_bitboards[k] && lsb(saved_bitboards[k]) % 8 > 3
            };

            if (previous_buckets != new_buckets)
            {
                to_update = white_bucket == previous_buckets.first;
            }
            else
            {
                to_update = new_mirrors.first == (lsb(previous_boards[K]) % 8 > 3);
            }

            if (!to_update)
            {
                for (int piece = 0; piece < 12; piece++)
                {
                    const auto old_white = saved_mirrors.first
                                               ? horizontal_mirror(saved_bitboards[piece])
                                               : saved_bitboards[piece];
                    const auto new_white = new_mirrors.first
                                               ? horizontal_mirror(new_bitboards[piece])
                                               : new_bitboards[piece];
                    auto white_adds = std::byteswap(new_white & ~old_white);

                    while (white_adds)
                    {
                        const int index_added = pop_lsb(white_adds);
                        const int input_index = index_added + piece * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter] = static_cast<int16_t>(saved_accumulators[iter] + network->
                                accumulator_weights[white_bucket][input_index][iter]);
                        }
                    }

                    auto white_subs = std::byteswap(old_white & ~new_white);

                    while (white_subs)
                    {
                        const int index_subbed = pop_lsb(white_subs);
                        const int input_index = index_subbed + piece * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter] = static_cast<int16_t>(saved_accumulators[iter] - network->
                                accumulator_weights[white_bucket][input_index][iter]);
                        }
                    }
                }

                update_from_move<white>(network, previous_accumulators, current_accumulators, stack_entry->adds,
                                        stack_entry->subs, new_mirrors, new_buckets);
                std::memcpy(current_accumulators, saved_accumulators, HL_SIZE * sizeof(int16_t));
                std::memcpy(&saved_accumulators[HL_SIZE], &current_accumulators[HL_SIZE], HL_SIZE * sizeof(int16_t));
            }
            else
            {
                for (int piece = 0; piece < 12; piece++)
                {
                    const auto old_black = saved_mirrors.second
                                               ? horizontal_mirror(saved_bitboards[piece])
                                               : saved_bitboards[piece];

                    const auto new_black = new_mirrors.second
                                               ? horizontal_mirror(new_bitboards[piece])
                                               : new_bitboards[piece];
                    auto black_adds = new_black & ~old_black;

                    while (black_adds)
                    {
                        const int index_added = pop_lsb(black_adds);
                        const int input_index = index_added + flip_color(piece) * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter + HL_SIZE] = static_cast<int16_t>(saved_accumulators[iter + HL_SIZE]
                                + network->accumulator_weights[black_bucket][input_index][iter]);
                        }
                    }

                    auto black_subs = old_black & ~new_black;

                    while (black_subs)
                    {
                        const int index_subbed = pop_lsb(black_subs);
                        const int input_index = index_subbed + flip_color(piece) * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter + HL_SIZE] = static_cast<int16_t>(saved_accumulators[iter + HL_SIZE]
                                - network->accumulator_weights[black_bucket][input_index][iter]);
                        }
                    }
                }
                update_from_move<black>(network, previous_accumulators, current_accumulators, stack_entry->adds,
                                        stack_entry->subs, new_mirrors, new_buckets);
                std::memcpy(&current_accumulators[HL_SIZE], &saved_accumulators[HL_SIZE], HL_SIZE * sizeof(int16_t));
                std::memcpy(saved_accumulators, current_accumulators, HL_SIZE * sizeof(int16_t));
            }

            std::memcpy(saved_bitboards, new_bitboards, 12 * sizeof(uint64_t));
        }

        stack_entry->is_dirty = false;
    }
}
