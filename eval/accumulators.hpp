#pragma once
#include "nnue.hpp"

inline Accumulators root_accumulators;
inline std::deque<Accumulator_entry> accumulator_stack;

inline uint8_t flip_color(const uint8_t piece)
{
    switch (piece) {
        case P: case N: case B: case R: case Q: case K:
            return piece + 6;
        case p: case n: case b: case r: case q: case k:
            return piece - 6;
        default:
            return piece;
    }
}

inline std::pair<uint16_t, uint16_t> input_index_of(const uint8_t p, const uint8_t sq)
{
    return {p * 64 + (sq ^ 56), flip_color(p) * 64 + sq};
}

inline void accumulators_add(const Network& network, Accumulators* accumulators, const uint8_t piece, const uint8_t sq)
{
    auto [white_add, black_add] = input_index_of(piece, sq);
    for (uint16_t i = 0; i < hl_size; i++) {
        (*accumulators)[white][i] += network.accumulator_weights[white_add][i];
        (*accumulators)[black][i] += network.accumulator_weights[black_add][i];
    }
}

//Quiet moves and non-capture promotions: 1 add and 1 sub.
inline void accumulators_addsub(const Network& network, Accumulators* accumulators, const std::pair<uint8_t, int16_t> &add, const std::pair<uint8_t, int16_t> &sub)
{
    auto [white_add, black_add] = input_index_of(add.first, add.second);
    auto [white_sub, black_sub] = input_index_of(sub.first, sub.second);

    for (uint16_t i = 0; i < hl_size; i++) {
        (*accumulators)[white][i] += network.accumulator_weights[white_add][i];
        (*accumulators)[white][i] -= network.accumulator_weights[white_sub][i];
        (*accumulators)[black][i] += network.accumulator_weights[black_add][i];
        (*accumulators)[black][i] -= network.accumulator_weights[black_sub][i];
    }
}

//Capture moves and capture promotions: One add and 2 subs.
//In case of promo-captures: the piece added and the piece subtracted are different.
inline void accumulators_addsub2(const Network& network, Accumulators* accumulators, const std::pair<uint8_t, int16_t> &add, const std::pair<uint8_t, int16_t> &sub1, const std::pair<uint8_t, int16_t> &sub2)
{
    auto [white_add, black_add] = input_index_of(add.first, add.second);
    auto [white_sub1, black_sub1] = input_index_of(sub1.first, sub1.second);
    auto [white_sub2, black_sub2] = input_index_of(sub2.first, sub2.second);

    for (uint16_t i = 0; i < hl_size; i++) {
        (*accumulators)[white][i] += network.accumulator_weights[white_add][i];
        (*accumulators)[white][i] -= network.accumulator_weights[white_sub1][i];
        (*accumulators)[white][i] -= network.accumulator_weights[white_sub2][i];

        (*accumulators)[black][i] += network.accumulator_weights[black_add][i];
        (*accumulators)[black][i] -= network.accumulator_weights[black_sub1][i];
        (*accumulators)[black][i] -= network.accumulator_weights[black_sub2][i];
    }
}

//Castling: 2 adds and 2 subs
inline void accumulators_add2sub2(const Network& network, Accumulators* accumulators, const std::pair<uint8_t, int16_t> &add1, const std::pair<uint8_t, int16_t> &add2, const std::pair<uint8_t, int16_t> &sub1, const std::pair<uint8_t, int16_t> &sub2)
    {
    auto [white_add1, black_add1] = input_index_of(add1.first, add1.second);
    auto [white_add2, black_add2] = input_index_of(add2.first, add2.second);
    auto [white_sub1, black_sub1] = input_index_of(sub1.first, sub1.second);
    auto [white_sub2, black_sub2] = input_index_of(sub2.first, sub2.second);

    for (uint16_t i = 0; i < hl_size; i++) {
        (*accumulators)[white][i] += network.accumulator_weights[white_add1][i];
        (*accumulators)[white][i] += network.accumulator_weights[white_add2][i];
        (*accumulators)[white][i] -= network.accumulator_weights[white_sub1][i];
        (*accumulators)[white][i] -= network.accumulator_weights[white_sub2][i];

        (*accumulators)[black][i] += network.accumulator_weights[black_add1][i];
        (*accumulators)[black][i] += network.accumulator_weights[black_add2][i];
        (*accumulators)[black][i] -= network.accumulator_weights[black_sub1][i];
        (*accumulators)[black][i] -= network.accumulator_weights[black_sub2][i];
    }
    }

inline void accumulators_refresh(const Network& network, Accumulators* accumulators, const Position& pos)
{
    memcpy((*accumulators)[white], network.accumulator_biases, hl_size * sizeof(int16_t));
    memcpy((*accumulators)[black], network.accumulator_biases, hl_size * sizeof(int16_t));

    for (int i = 0; i < 12; i++) {
        uint64_t board = pos.boards[i];
        while (board) {
            accumulators_add(network, accumulators, i, least_significant_one(board));
            board &= board - 1;
        }
    }
}

inline void accumulator_stack_update(const Network& network)
{
    auto idx = accumulator_stack.size() - 1;
    while (accumulator_stack[idx - 1].is_dirty) idx--;

    for (; idx < accumulator_stack.size(); idx++) {
        memcpy(&accumulator_stack[idx].accumulators, &accumulator_stack[idx - 1].accumulators, 2 * hl_size * sizeof(int16_t));
        const auto entry = &accumulator_stack[idx];
        auto accumulators = &entry->accumulators;
        //At least two subs: A capture, promo-capture or castling.
        if (entry->subs[1].second != -1) {
            //Castling
            if (entry->adds[1].second != - 1) {
                accumulators_add2sub2(network, accumulators, entry->adds[0], entry->adds[1], entry->subs[0], entry->subs[1]);
            }
            //Capture or promo-capture
            else {
                accumulators_addsub2(network, accumulators, entry->adds[0], entry->subs[0], entry->subs[1]);
            }
        }
        else {
            accumulators_addsub(network, accumulators, entry->adds[0], entry->subs[0]);
        }

        entry->is_dirty = false;
    }
}