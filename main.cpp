#include "movegen.hpp"
#include "uci.hpp"

int main()
{
    generate_lines();
    Zobrist::generate_keys();

    UCI::process();
}
