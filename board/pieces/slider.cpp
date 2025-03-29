#include "slider.h"
#include "bishop.h"
#include <bit>

constexpr uint64_t occupancy_board(const uint8_t set_index, const uint8_t mask_bits_count, uint64_t attack_board)
{
    uint64_t occupancy = 0;
    for (uint8_t count = 0; count < mask_bits_count; count++) {
        const uint8_t index = std::popcount((attack_board & -attack_board) - 1);
        attack_board = attack_board & (attack_board - 1);
        if (set_index & (1ull << count)) {
            occupancy |= 1ull << index;
        }
    }
    return occupancy;
}

uint64_t get_bishop_attack(const uint8_t index, uint64_t occupancy)
{
    const uint64_t* table = bishop_magics[index].attacks;
    occupancy &= bishop_magics[index].mask;
    occupancy *= bishop_magics[index].magic;
    occupancy >>= bishop_magics[index].shift;
    return table[occupancy];
}

uint64_t get_bishop_attack_index(const uint8_t index, uint64_t occupancy)
{
    occupancy &= bishop_magics[index].mask;
    occupancy *= bishop_magics[index].magic;
    occupancy >>= bishop_magics[index].shift;
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

uint64_t get_rook_attack(const uint8_t index, uint64_t occupancy)
{
    const uint64_t* table = rook_magics[index].attacks;
    occupancy &= rook_magics[index].mask;
    occupancy *= rook_magics[index].magic;
    occupancy >>= rook_magics[index].shift;
    return table[occupancy];
}

uint64_t get_rook_attack_index(const uint8_t index, uint64_t occupancy)
{
    occupancy &= rook_magics[index].mask;
    occupancy *= rook_magics[index].magic;
    occupancy >>= rook_magics[index].shift;
    return occupancy;
}

uint64_t get_queen_attack(const uint8_t index, uint64_t occupancy)
{
    auto temp = occupancy;
    const uint64_t* b_table = rook_magics[index].attacks;
    temp &= rook_magics[index].mask;
    temp *= rook_magics[index].magic;
    temp >>= rook_magics[index].shift;

    const uint64_t* r_table = bishop_magics[index].attacks;
    occupancy &= bishop_magics[index].mask;
    occupancy *= bishop_magics[index].magic;
    occupancy >>= bishop_magics[index].shift;
    return b_table[temp] | r_table[occupancy];
}