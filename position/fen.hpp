#pragma once
#include <cstdint>
#include <string>

int algebraic_to_num(const std::string &algebraic);
std::string num_to_algebraic(uint8_t sq);

int fen_parse(std::string fen);
