#include "fen.h"
#include "move.h"
#include "position.h"
#include "board/bitboard.h"

int main()
{
    initialize();
    std::cout << fen_parse("7b/2r5/7k/8/1p4N1/2K4r/8/1n2q3 b - - 0 1") << "\n";
}

