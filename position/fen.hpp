#pragma once

struct Position;

int fen_parse(Position& position, std::string_view fen);