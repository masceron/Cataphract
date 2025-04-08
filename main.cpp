#include "uci.hpp"
#include "eval/transposition.hpp"

int main()
{
    generate_lines();
    Zobrist::generate_keys();
    Table::clear();

    UCI::process();

}
