#include "uci.hpp"
#include "eval/transposition.hpp"
#include "search/see.hpp"

int main()
{
    generate_lines();
    Zobrist::generate_keys();
    Table::clear();

    UCI::process();

}
