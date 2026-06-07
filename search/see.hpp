#pragma once

struct Move;
struct Position;
enum Piece : uint8_t;

int value_of(Piece piece);
int static_exchange_evaluation(const Position& pos, Move capture);