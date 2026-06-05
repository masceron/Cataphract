#pragma once

#include "../position/position.hpp"

int value_of(Piece piece);
int static_exchange_evaluation(const Position& pos, const Move capture);