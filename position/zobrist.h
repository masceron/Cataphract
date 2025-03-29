#pragma once
#include <cstdint>
#include <random>
#include <array>
#include <algorithm>

enum Zobrist_Non_Pawn_Key: uint8_t
{
     z_N, z_B, z_R, z_Q, z_K, z_n, z_b, z_r, z_q, z_k
};

enum Zobrist_Pawn_key: uint8_t
{
    z_P, z_p
};

struct Zobrist
{
    std::array<std::array<uint64_t, 64>, 10> non_pawn_keys;
    std::array<std::array<uint64_t, 48>, 2> pawn_keys;
    std::array<uint64_t, 16> castling_keys;
    uint64_t side_key;
    uint64_t en_passant_key;
    Zobrist()
    {
        std::random_device rd;
        std::mt19937_64 gnr(rd());
        std::uniform_int_distribution<uint64_t> dis;
        auto gen = [&] {return dis(gnr);};
        std::ranges::generate(castling_keys.begin(), castling_keys.end(), gen);
        for (auto& i: non_pawn_keys) {
            std::ranges::generate(i.begin(), i.end(), gen);
        }
        for (auto& i: pawn_keys) {
            std::ranges::generate(i.begin(), i.end(), gen);
        }
        side_key = dis(gnr);
        en_passant_key = dis(gnr);
    }
};

inline Zobrist zobrist_keys{};


