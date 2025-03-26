#pragma once

#include <cstdint>
#include "pieces/slider.h"

inline uint64_t lines_between[2016]{};
inline uint64_t lines_intersect[2016]{};

constexpr std::array<uint16_t, 63> line_offsets = {
    0, 63, 125, 186, 246, 305, 363, 420, 476, 531, 585, 638, 690, 741, 791, 840, 888, 935, 981, 1026, 1070, 1113, 1155, 1196, 1236, 1275, 1313, 1350, 1386, 1421, 1455, 1488, 1520, 1551, 1581, 1610, 1638, 1665, 1691, 1716, 1740, 1763, 1785, 1806, 1826, 1845, 1863, 1880, 1896, 1911, 1925, 1938, 1950, 1961, 1971, 1980, 1988, 1995, 2001, 2006, 2010, 2013, 2015
};

void generate_lines();

uint64_t get_line_between(uint8_t from, uint8_t to);
uint64_t get_line_intersect(uint8_t from, uint8_t to);