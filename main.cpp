#include "slider.h"
#include "board/bitboard.h"

int main()
{
    uint64_t occ = 282643207815424;
    print_board(get_queen_attack(32, occ));



    std::cin.get();
}
