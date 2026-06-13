#pragma once

#include <cstdint>
#include <utility>

uint8_t flip_color(uint8_t piece);
int nnue_index(uint8_t piece);
std::pair<int, int> input_index_of(uint8_t piece, uint8_t square, const std::pair<bool, bool>& mirrors);
uint64_t horizontal_mirror(uint64_t board);
std::pair<uint8_t, uint8_t> get_buckets(const std::array<uint64_t, 14>& boards);