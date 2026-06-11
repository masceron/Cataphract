#include "../board/attacks.hpp"
#include "../board/slider.hpp"
#include "cuckoo.hpp"
#include "zobrist.hpp"

namespace Cuckoo
{
    uint64_t np_piece_attacks(const int from, const int to, const Piece piece)
    {
        const uint64_t to_board = 1ull << to;
        switch (type_of(piece))
        {
        case Knight:
            return knight_attack_tables[from] & to_board;
        case King:
            return king_attack_tables[from] & to_board;
        case Bishop:
            return get_bishop_attack(from, 0ull) & to_board;
        case Rook:
            return get_rook_attack(from, 0ull) & to_board;
        case Queen:
            return get_queen_attack(from, 0ull) & to_board;
        default:
            return 0ull;
        }
    }

    uint64_t hash1(const uint64_t key)
    {
        return key & 0x1fffull;
    }

    uint64_t hash2(const uint64_t key)
    {
        return (key >> 16) & 0x1fffull;
    }

    void init()
    {
        for (int piece = 1; piece < 14; piece++)
        {
            if (piece == p || piece == 6 || piece == 7) continue;
            for (int sq0 = 0; sq0 < 64; sq0++)
            {
                for (int sq1 = sq0 + 1; sq1 < 64; sq1++)
                {
                    if (np_piece_attacks(sq0, sq1, static_cast<Piece>(piece)))
                    {
                        Move move(sq0, sq1, MoveFlag::quiet_move);
                        uint64_t key = Zobrist::piece_keys[piece][sq0] ^ Zobrist::piece_keys[piece][sq1] ^
                            Zobrist::side_key;

                        uint64_t slot = hash1(key);
                        while (true)
                        {
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
