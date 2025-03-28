#include "fen.h"
#include "move.h"
#include "position.h"
#include "board/bitboard.h"

int main()
{
    initialize();
    fen_parse("startpos");
    print_board();
}

