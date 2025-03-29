#include "lines.h"
#include "pieces/slider.h"

void generate_lines()
{
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            if (const int dif = abs(i - j); (dif < 8 && i / 8 == j / 8) || (i - j) % 8 == 0) {
                lines_between[i][j] = (get_rook_attack(i, 1ull << j) & get_rook_attack(j, 1ull << i)) | (1ull << i);
                lines_intersect[i][j] = (get_rook_attack(i, 0) & get_rook_attack(j, 0)) | (1ull << i) | (1ull << j);
            } else if (dif % 9 == 0 || dif % 7 == 0) {
                lines_between[i][j] = (get_bishop_attack(i, 1ull << j) & get_bishop_attack(j, 1ull << i)) | (1ull << i);
                lines_intersect[i][j] = ((get_bishop_attack(i, 0) & get_bishop_attack(j, 0)) | (1ull << i) | (1ull << j));
            }
            else {
                lines_between[i][j] = 0ull;
                lines_intersect[i][j] = 0ull;
            }
        }
    }
}