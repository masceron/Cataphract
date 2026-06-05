#pragma once

#include <cstdint>

uint64_t get_bishop_attack(int index, uint64_t occupancy);
uint64_t get_rook_attack(int index, uint64_t occupancy);
uint64_t get_queen_attack(int index, uint64_t occupancy);