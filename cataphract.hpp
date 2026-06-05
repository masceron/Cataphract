#pragma once
#include "position/movegen.hpp"
#include "position/position.hpp"

namespace Cataphract
{
    void new_game();
    void start();
    void process_move(Position& position, std::string_view move, MoveList& list);
    void set_board(std::string_view fen);
}
