#include "uci.hpp"
#include "eval/transposition.hpp"

int main()
{
    generate_lines();
    Zobrist::generate_keys();
    TT::clear();
    History::clear();

    UCI::process();
}