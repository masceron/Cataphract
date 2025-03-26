#include "fen.h"
#include "position.h"
#include "board/bitboard.h"

int main()
{
    initialize();
    std::cout << fen_parse("6k1/8/5q2/3N4/8/2P5/1K3Rrr/8 b - - 0 1") << "\n";
    print_board(get_pinned_board_of(white));
}
