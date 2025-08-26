#include "slider.hpp"
#include "bishop.hpp"
#include <bit>

constexpr uint64_t occupancy_board(const int set_index, const int mask_bits_count, uint64_t attack_board)
{
    uint64_t occupancy = 0;
    for (int count = 0; count < mask_bits_count; count++) {
        const int index = std::popcount((attack_board & -attack_board) - 1);
        attack_board = attack_board & (attack_board - 1);
        if (set_index & (1ull << count)) {
            occupancy |= 1ull << index;
        }
    }
    return occupancy;
}

std::array<Magic, 64> generate_magics(const bool is_bishop)
{
    constexpr std::array<Magic, 64> magics = {};
    int size = 0, count = 0;
    std::array<int, 4096> epoch{};
    std::array<uint64_t, 4096> occupancy{};
    std::array<uint64_t, 4096> references{};
    for (int sq = 0; sq < 64; sq++) {
        uint64_t temp = 0;
        const uint64_t mask = is_bishop ? mask_bishop_attack(sq) : mask_rook_attack(sq);
        const Magic &magic = magics[sq];
        magic.shift = 64 - std::popcount(mask);
        magic.mask = mask;
        if (sq == 0)
            magic.attacks = is_bishop ? bishop_table.data() : rook_table.data();
        else
            magic.attacks = magics[sq - 1].attacks + size;
        size = 0;
        do {
            occupancy[size] = temp;
            references[size] = is_bishop ? generate_bishop_attack(sq, temp) : generate_rook_attack(sq, temp);
            size++;
            temp = (temp - mask) & mask;
        } while(temp);
        for (int i = 0; i < size; i++) {
            const uint64_t magic_number = is_bishop ? bishop_magic_numbers[sq] : rook_magic_numbers[sq];
            // uint64_t magic_number = 0;
            // while (std::popcount((magic_number * mask) >> 56) < 6) {
            //     magic_number = random_uint64_few_bits();
            // }

            magic.magic = magic_number;
            for (++count, i = 0; i < size; ++i) {
                if (const uint64_t index = ((occupancy[i] & mask) * magic_number) >> magic.shift; epoch[index] < count) {
                    epoch[index] = count;
                    magic.attacks[index] = references[i];
                }
                else if (magic.attacks[index] != references[i])
                    break;
            }
        }
    }
    return magics;
}
