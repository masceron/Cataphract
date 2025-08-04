#pragma once
#include <cstdint>

#include "move.hpp"
#include "zobrist.hpp"
#include "../board/pieces/king.hpp"
#include "../board/pieces/knight.hpp"
#include "../board/pieces/slider.hpp"


namespace Cuckoo
{
    inline std::vector<uint64_t> cuckoo_key(8192, 0);
    inline std::vector cuckoo_move(8192, move_none);

    inline uint64_t np_piece_attacks(const uint8_t from, const uint8_t to, const uint8_t piece)
    {
        const uint64_t to_board = 1ull << to;
        switch (piece) {
            case N: case n:
                return knight_attack_tables[from] & to_board;
            case K: case k:
                return king_attack_tables[from] & to_board;
            case B: case b:
                return get_bishop_attack(from, 0ull) & to_board;
            case R: case r:
                return get_rook_attack(from, 0ull) & to_board;
            case Q: case q:
                return get_queen_attack(from, 0ull) & to_board;
            default:
                return 0ull;
        }
    }

    inline uint64_t hash1(const uint64_t key)
    {
        return key & 0x1fffull;
    }

    inline uint64_t hash2(const uint64_t key)
    {
        return (key >> 16) & 0x1fffull;
    }

    inline void init()
    {
        for (uint8_t piece = 1; piece < 12; piece++) {
            if (piece == p) continue;
            for (uint8_t sq0 = 0; sq0 < 64; sq0++) {
                for (uint8_t sq1 = sq0 + 1; sq1 < 64; sq1++) {
                    if (np_piece_attacks(sq0, sq1, piece)) {
                        Move move(sq0, sq1, quiet_move);
                        uint64_t key = Zobrist::piece_keys[piece][sq0] ^ Zobrist::piece_keys[piece][sq1] ^ Zobrist::side_key;

                        uint64_t slot = hash1(key);
                        while (true) {
                            std::swap(cuckoo_key[slot], key);
                            std::swap(cuckoo_move[slot], move);

                            if (!move) break;

                            slot = slot == hash1(key) ? hash2(key) : hash1(key);
                        }
                    }
                }
            }
        }
    }
}
