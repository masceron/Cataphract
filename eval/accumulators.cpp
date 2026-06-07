#include <cstring>
#include <bit>

#include "utils.hpp"
#include "accumulators.hpp"
#include "../position/move.hpp"
#include "../position/position.hpp"

void AccumulatorEntry::mark_changes(const Position& pos, const Move move)
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
    const Piece moved_piece = pos.piece_on[to];
    std::memcpy(bitboards.data(), &pos.boards, sizeof(bitboards));
    const bool just_moved = !pos.side_to_move;

    if (type_of(moved_piece) == King)
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

AccumulatorStack::AccumulatorStack() : stack{}, size(0)
{
}

void AccumulatorStack::push(const Position& pos, const Move move)
{
    stack[size].mark_changes(pos, move);
    ++size;
}

void AccumulatorStack::push()
{
    ++size;
}

void AccumulatorStack::clear()
{
    size = 0;
}

void AccumulatorStack::pop()
{
    --size;
}

AccumulatorEntry& AccumulatorStack::operator[](const int idx)
{
    return stack[idx];
}

void accumulators_set(const Network& __restrict network, const std::array<uint64_t, 14>& boards,
                      int16_t* __restrict accumulators)
{
    std::memcpy(accumulators, network.accumulator_biases, HL_SIZE * sizeof(int16_t));
    std::memcpy(&accumulators[HL_SIZE], network.accumulator_biases, HL_SIZE * sizeof(int16_t));

    const std::pair mirror = {lsb(boards[K]) % 8 > 3, lsb(boards[k]) % 8 > 3};
    const auto [white_bucket, black_bucket] = get_buckets(boards);

    for (int i = 0; i < 14; i++)
    {
        uint64_t board = boards[i];

        while (board)
        {
            auto [white_add, black_add] = input_index_of(i, pop_lsb(board), mirror);
            for (int iter = 0; iter < HL_SIZE; iter++)
            {
                accumulators[iter] = static_cast<int16_t>(accumulators[iter]
                    + network.accumulator_weights[white_bucket][white_add][iter]);
                accumulators[iter + HL_SIZE] = static_cast<int16_t>(accumulators[iter + HL_SIZE] + network.
                    accumulator_weights[black_bucket][black_add][iter]);
            }
        }
    }
}

template <const int exclude>
void accumulators_addsub(const Network& __restrict network, const int16_t* __restrict prev,
                         int16_t* __restrict accs,
                         const std::pair<uint8_t, int8_t>* add,
                         const std::pair<uint8_t, int8_t>* sub, const std::pair<bool, bool>& mirrors,
                         const std::pair<uint8_t, uint8_t>& buckets)
{
    auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, mirrors);
    auto [white_sub, black_sub] = input_index_of(sub[0].first, sub[0].second, mirrors);

    for (int iter = 0; iter < HL_SIZE; iter++)
    {
        if constexpr (exclude != white)
        {
            accs[iter] = prev[iter]
                + network.accumulator_weights[buckets.first][white_add][iter]
                - network.accumulator_weights[buckets.first][white_sub][iter];
        }

        if constexpr (exclude != black)
        {
            accs[iter + HL_SIZE] = prev[iter + HL_SIZE]
                + network.accumulator_weights[buckets.second][black_add][iter]
                - network.accumulator_weights[buckets.second][black_sub][iter];
        }
    }
}

template <const int exclude>
void accumulators_addsub2(const Network& __restrict network, const int16_t* __restrict prev,
                          int16_t* __restrict accs,
                          const std::pair<uint8_t, int8_t>* add,
                          const std::pair<uint8_t, int8_t>* sub, const std::pair<bool, bool>& mirrors,
                          const std::pair<uint8_t, uint8_t>& buckets)
{
    auto [white_add, black_add] = input_index_of(add[0].first, add[0].second, mirrors);
    auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, mirrors);
    auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, mirrors);

    for (int iter = 0; iter < HL_SIZE; iter++)
    {
        if constexpr (exclude != white)
        {
            accs[iter] = prev[iter]
                + network.accumulator_weights[buckets.first][white_add][iter]
                - network.accumulator_weights[buckets.first][white_sub1][iter]
                - network.accumulator_weights[buckets.first][white_sub2][iter];
        }

        if constexpr (exclude != black)
        {
            accs[iter + HL_SIZE] = prev[iter + HL_SIZE]
                + network.accumulator_weights[buckets.second][black_add][iter]
                - network.accumulator_weights[buckets.second][black_sub1][iter]
                - network.accumulator_weights[buckets.second][black_sub2][iter];
        }
    }
}

template <const int exclude>
void accumulators_add2sub2(const Network& __restrict network, const int16_t* __restrict prev,
                           int16_t* __restrict accs,
                           const std::pair<uint8_t, int8_t>* add,
                           const std::pair<uint8_t, int8_t>* sub,
                           const std::pair<bool, bool>& mirrors,
                           const std::pair<uint8_t, uint8_t>& buckets
)
{
    auto [white_add1, black_add1] = input_index_of(add[0].first, add[0].second, mirrors);
    auto [white_add2, black_add2] = input_index_of(add[1].first, add[1].second, mirrors);
    auto [white_sub1, black_sub1] = input_index_of(sub[0].first, sub[0].second, mirrors);
    auto [white_sub2, black_sub2] = input_index_of(sub[1].first, sub[1].second, mirrors);

    for (int iter = 0; iter < HL_SIZE; iter++)
    {
        if constexpr (exclude != white)
        {
            accs[iter] = prev[iter]
                + network.accumulator_weights[buckets.first][white_add1][iter]
                + network.accumulator_weights[buckets.first][white_add2][iter]
                - network.accumulator_weights[buckets.first][white_sub1][iter]
                - network.accumulator_weights[buckets.first][white_sub2][iter];
        }

        if constexpr (exclude != black)
        {
            accs[iter + HL_SIZE] = prev[iter + HL_SIZE]
                + network.accumulator_weights[buckets.second][black_add1][iter]
                + network.accumulator_weights[buckets.second][black_add2][iter]
                - network.accumulator_weights[buckets.second][black_sub1][iter]
                - network.accumulator_weights[buckets.second][black_sub2][iter];
        }
    }
}

template <const int exclude>
void update_from_move(const Network& __restrict network, int16_t* __restrict prev, int16_t* __restrict cur,
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
            accumulators_add2sub2<exclude>(network, prev, cur,
                                           add, sub, mirrors,
                                           buckets);
        }
        //Capture or promo-capture
        else
        {
            accumulators_addsub2<exclude>(network, prev, cur, add,
                                          sub, mirrors,
                                          buckets);
        }
    }
    else
    {
        accumulators_addsub<exclude>(network, prev, cur, add,
                                     sub, mirrors,
                                     buckets);
    }
}

void accumulator_stack_update(const Network& __restrict network, AccumulatorStack& accumulator_stack)
{
    auto idx = accumulator_stack.size - 1;
    while (accumulator_stack[idx - 1].is_dirty) idx--;
    auto& finny_table = accumulator_stack.finny_table;

    for (; idx < accumulator_stack.size; idx++)
    {
        auto& stack_entry = accumulator_stack[idx];
        auto& new_bitboards = stack_entry.bitboards;
        const auto new_buckets = get_buckets(new_bitboards);
        const std::pair new_mirrors = {
            lsb(new_bitboards[K]) % 8 > 3, lsb(new_bitboards[k]) % 8 > 3
        };
        const auto current_accumulators = stack_entry.accumulators;

        auto& previous_entry = accumulator_stack[idx - 1];
        auto& previous_boards = previous_entry.bitboards;
        auto& previous_accumulators = accumulator_stack[idx - 1].accumulators;

        if (!stack_entry.require_rebuild)
        {
            update_from_move<-1>(network, previous_accumulators, current_accumulators, stack_entry.adds,
                                 stack_entry.subs, new_mirrors, new_buckets);
        }
        else
        {
            const auto previous_buckets = get_buckets(previous_boards);
            const auto [white_bucket, black_bucket] = new_buckets;
            auto& saved_entry = finny_table[white_bucket][black_bucket];
            auto& saved_bitboards = saved_entry.bitboards;
            auto& saved_accumulators = saved_entry.accumulators;

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
                for (int piece = 0; piece < 14; piece++)
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
                        const int input_index = index_added + nnue_index(piece) * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter] = static_cast<int16_t>(saved_accumulators[iter] + network.
                                accumulator_weights[white_bucket][input_index][iter]);
                        }
                    }

                    auto white_subs = std::byteswap(old_white & ~new_white);

                    while (white_subs)
                    {
                        const int index_subbed = pop_lsb(white_subs);
                        const int input_index = index_subbed + nnue_index(piece) * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter] = static_cast<int16_t>(saved_accumulators[iter] - network.
                                accumulator_weights[white_bucket][input_index][iter]);
                        }
                    }
                }

                update_from_move<white>(network, previous_accumulators, current_accumulators, stack_entry.adds,
                                        stack_entry.subs, new_mirrors, new_buckets);
                std::memcpy(current_accumulators, saved_accumulators, HL_SIZE * sizeof(int16_t));
                std::memcpy(&saved_accumulators[HL_SIZE], &current_accumulators[HL_SIZE], HL_SIZE * sizeof(int16_t));
            }
            else
            {
                for (int piece = 0; piece < 14; piece++)
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
                        const int input_index = index_added + nnue_index(flip_color(piece)) * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter + HL_SIZE] = static_cast<int16_t>(saved_accumulators[iter + HL_SIZE]
                                + network.accumulator_weights[black_bucket][input_index][iter]);
                        }
                    }

                    auto black_subs = old_black & ~new_black;

                    while (black_subs)
                    {
                        const int index_subbed = pop_lsb(black_subs);
                        const int input_index = index_subbed + nnue_index(flip_color(piece)) * 64;
                        for (int iter = 0; iter < HL_SIZE; iter++)
                        {
                            saved_accumulators[iter + HL_SIZE] = static_cast<int16_t>(saved_accumulators[iter + HL_SIZE]
                                - network.accumulator_weights[black_bucket][input_index][iter]);
                        }
                    }
                }
                update_from_move<black>(network, previous_accumulators, current_accumulators, stack_entry.adds,
                                        stack_entry.subs, new_mirrors, new_buckets);
                std::memcpy(&current_accumulators[HL_SIZE], &saved_accumulators[HL_SIZE], HL_SIZE * sizeof(int16_t));
                std::memcpy(saved_accumulators, current_accumulators, HL_SIZE * sizeof(int16_t));
            }

            std::memcpy(saved_bitboards.data(), new_bitboards.data(), sizeof(new_bitboards));
        }

        stack_entry.is_dirty = false;
    }
}
