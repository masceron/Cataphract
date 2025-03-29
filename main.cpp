#include <iostream>

#include "fen.hpp"
#include "position.hpp"


int main()
{
    initialize();
    fen_parse("8/1r6/5q2/7k/1P6/8/1K2B1r1/8 b - - 0 1");
    print_board();

    std::cout << position.state->key;

}