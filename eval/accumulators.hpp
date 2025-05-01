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

inline std::pair<uint16_t, uint16_t> input_index_of(const uint8_t p, const uint8_t sq, const std::pair<bool, bool>& is_mirrored)
{
    uint8_t wsq = sq, bsq = sq;
    if (is_mirrored.first) wsq ^= 7;
    if (is_mirrored.second) bsq ^= 7;
    return {p * 64 + (wsq ^ 56), flip_color(p) * 64 + bsq};
}

inline void accumulators_add(Network* network, Accumulators* accumulators, const uint8_t piece, const uint8_t sq, const std::pair<bool, bool>& is_mirrored)
{
    auto [white_add, black_add] = input_index_of(piece, sq, is_mirrored);

    for (int iter = 0; iter < hl_size; iter++) {
        (*accumulators)[white][iter] += network->accumulator_weights[white_add][iter];
        (*accumulators)[black][iter] += network->accumulator_weights[black_add][iter];
    }
}

//Quiet moves and non-capture promotions: 1 add and 1 sub.
template <const int8_t excluded>
void accumulators_addsub(Network* network, Accumulators* previous, Accumulators* current, const std::pair<uint8_t, int16_t> &add, const std::pair<uint8_t, int16_t> &sub, const std::pair<bool, bool>& is_mirrored)
{
    auto [white_add, black_add] = input_index_of(add.first, add.second, is_mirrored);
    auto [white_sub, black_sub] = input_index_of(sub.first, sub.second, is_mirrored);


    for (int iter = 0; iter < hl_size; iter++) {
        if constexpr (excluded != white) {
            (*current)[white][iter] = (*previous)[white][iter]
                                    + network->accumulator_weights[white_add][iter]
                                    - network->accumulator_weights[white_sub][iter];
        }
        if constexpr (excluded != black) {
            (*current)[black][iter] = (*previous)[black][iter]
                                    + network->accumulator_weights[black_add][iter]
                                    - network->accumulator_weights[black_sub][iter];
        }
    }
}

//Capture moves and capture promotions: One add and 2 subs.
//In case of promo-captures: the piece added and the piece subtracted are different.
template <const int8_t excluded>
void accumulators_addsub2(Network* network, Accumulators* previous, Accumulators* current, const std::pair<uint8_t, int16_t> &add, const std::pair<uint8_t, int16_t> &sub1, const std::pair<uint8_t, int16_t> &sub2, const std::pair<bool, bool>& is_mirrored)
{
    auto [white_add, black_add] = input_index_of(add.first, add.second, is_mirrored);
    auto [white_sub1, black_sub1] = input_index_of(sub1.first, sub1.second, is_mirrored);
    auto [white_sub2, black_sub2] = input_index_of(sub2.first, sub2.second, is_mirrored);


    for (uint16_t iter = 0; iter < hl_size; iter++) {
        if constexpr (excluded != white) {
            (*current)[white][iter] = (*previous)[white][iter]
                                    + network->accumulator_weights[white_add][iter]
                                    - network->accumulator_weights[white_sub1][iter]
                                    - network->accumulator_weights[white_sub2][iter];
        }

        if constexpr (excluded != black) {
            (*current)[black][iter] = (*previous)[black][iter]
                                    + network->accumulator_weights[black_add][iter]
                                    - network->accumulator_weights[black_sub1][iter]
                                    - network->accumulator_weights[black_sub2][iter];
        }
    }
}

//Castling: 2 adds and 2 subs
template<const int8_t excluded>
void accumulators_add2sub2(Network* network, Accumulators* previous, Accumulators* current, const std::pair<uint8_t, int16_t> &add1, const std::pair<uint8_t, int16_t> &add2, const std::pair<uint8_t, int16_t> &sub1, const std::pair<uint8_t, int16_t> &sub2, const std::pair<bool, bool>& is_mirrored)
{
    auto [white_add1, black_add1] = input_index_of(add1.first, add1.second, is_mirrored);
    auto [white_add2, black_add2] = input_index_of(add2.first, add2.second, is_mirrored);
    auto [white_sub1, black_sub1] = input_index_of(sub1.first, sub1.second, is_mirrored);
    auto [white_sub2, black_sub2] = input_index_of(sub2.first, sub2.second, is_mirrored);


    for (int iter = 0; iter < hl_size; iter++) {
        if constexpr (excluded != white) {
            (*current)[white][iter] = (*previous)[white][iter]
                                    + network->accumulator_weights[white_add1][iter]
                                    + network->accumulator_weights[white_add2][iter]
                                    - network->accumulator_weights[white_sub1][iter]
                                    - network->accumulator_weights[white_sub2][iter];
        }

        if constexpr (excluded != black) {
            (*current)[black][iter] = (*previous)[black][iter]
                                    + network->accumulator_weights[black_add1][iter]
                                    + network->accumulator_weights[black_add2][iter]
                                    - network->accumulator_weights[black_sub1][iter]
                                    - network->accumulator_weights[black_sub2][iter];
        }
    }
}

inline void accumulators_set(Network* network, Accumulators* accumulators, const Position& pos)
{
    memcpy((*accumulators)[white], network->accumulator_biases, hl_size * sizeof(int16_t));
    memcpy((*accumulators)[black], network->accumulator_biases, hl_size * sizeof(int16_t));
    const std::pair mirror = {least_significant_one(pos.boards[K]) % 8 >= 4, least_significant_one(pos.boards[k]) % 8 >= 4};

    for (int i = 0; i < 12; i++) {
        uint64_t board = pos.boards[i];
        while (board) {
            accumulators_add(network, accumulators, i, least_significant_one(board), mirror);
            board &= board - 1;
        }
    }
}

inline void accumulator_stack_update(Network* network)
{
    auto idx = accumulator_stack.size() - 1;
    while (accumulator_stack[idx - 1].is_dirty) idx--;

    for (; idx < accumulator_stack.size(); idx++) {
        const auto entry = &accumulator_stack[idx];
        const auto previous_entry = &accumulator_stack[idx - 1];
        const auto previous_accumulators = &previous_entry->accumulators;
        const auto current_accumulators = &entry->accumulators;
        uint8_t excluded = 2;
        if (entry->is_mirrored.first != previous_entry->is_mirrored.first) {
            excluded = 0;
        }
        else if (entry->is_mirrored.second != previous_entry->is_mirrored.second) {
            excluded = 1;
        }
        //At least two subs: A capture, promo-capture or castling.
        if (entry->subs[1].second != -1) {
            //Castling
            if (entry->adds[1].second != - 1) {
                switch (excluded) {
                    case 0:
                        accumulators_add2sub2<white>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->adds[1], entry->subs[0], entry->subs[1], entry->is_mirrored);
                        break;
                    case 1:
                        accumulators_add2sub2<black>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->adds[1], entry->subs[0], entry->subs[1], entry->is_mirrored);
                        break;
                    default:
                        accumulators_add2sub2<-1>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->adds[1], entry->subs[0], entry->subs[1], entry->is_mirrored);
                        break;
                }
            }
            //Capture or promo-capture
            else {
                switch (excluded) {
                    case 0:
                        accumulators_addsub2<white>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->subs[0], entry->subs[1], entry->is_mirrored);
                        break;
                    case 1:
                        accumulators_addsub2<black>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->subs[0], entry->subs[1], entry->is_mirrored);
                        break;
                    default:
                        accumulators_addsub2<-1>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->subs[0], entry->subs[1], entry->is_mirrored);
                        break;
                }}
        }
        else {
            switch (excluded) {
                case white:
                    accumulators_addsub<white>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->subs[0], entry->is_mirrored);
                    break;
                case black:
                    accumulators_addsub<black>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->subs[0], entry->is_mirrored);
                    break;
                default:
                    accumulators_addsub<-1>(network, previous_accumulators, current_accumulators, entry->adds[0], entry->subs[0], entry->is_mirrored);
                    break;
            }
        }
        entry->is_dirty = false;
    }
}