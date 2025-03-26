#include "lines.h"

void generate_lines()
{
    int count = 0;
    for (int i = 63; i >= 1; i--) {
        for (int to = 64 - i ; to <= 63; to++, count++) {
            if (const int from = 63 - i; (from / 8 == to / 8 && abs(from - to) < 8) || (from - to) % 8 == 0){
                lines_between[count] = get_rook_attack(from, 1ull << to) & get_rook_attack(to, 1ull << from);
                lines_intersect[count] = (get_rook_attack(from, 0ull) | (1ull << from)) & (get_rook_attack(to, 0ull) | (1ull << to));
            }
            else if (abs(to - from) % 7 == 0 || abs(to - from) % 9 == 0) {
                lines_between[count] = get_bishop_attack(from, 1ull << to) & get_bishop_attack(to, 1ull << from);
                lines_intersect[count] = (get_bishop_attack(from, 0ull) | (1ull << from)) & (get_bishop_attack(to, 0ull) | (1ull << to));
            }
            else {
                lines_between[count] = 0ull;
                lines_intersect[count] = 0ull;
            }
        }
    }
}

uint64_t get_line_between(uint8_t from, uint8_t to)
{
    if (from > to) {
        const uint8_t temp = from;
        from = to;
        to = temp;
    }
    return lines_between[line_offsets[from] + to - from - 1];
}

uint64_t get_line_intersect(uint8_t from, uint8_t to)
{
    if (from > to) {
        const uint8_t temp = from;
        from = to;
        to = temp;
    }
    return lines_intersect[line_offsets[from] + to - from - 1];
}