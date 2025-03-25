#include "fen.h"
#include "board/pieces/slider.h"
#include "board/bitboard.h"

int main()
{
    std::cout << fen_parse("startpos") << "\n";

    std::cin.get();
}
