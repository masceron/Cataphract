#pragma once
#include <cstdint>
#include <random>
#include <array>
#include <algorithm>

enum Zobrist_Non_Pawn_Key: uint8_t
{
     z_N, z_B, z_R, z_Q, z_K, z_n, z_b, z_r, z_q, z_k, z_err
};

enum Zobrist_Pawn_key: uint8_t
{
    z_P, z_p
};

inline Zobrist_Non_Pawn_Key pieces_to_zobrist_key(Pieces piece)
{
    switch (piece) {
        case N:
            return z_N;
        case B:
            return z_B;
        case R:
            return z_R;
        case Q:
            return z_Q;
        case K:
            return z_K;
        case n:
            return z_n;
        case b:
            return z_b;
        case r:
            return z_r;
        case q:
            return z_q;
        case k:
            return z_k;
        default:
            return z_err;
    }
}

struct Zobrist
{
    std::array<std::array<uint64_t, 64>, 10> non_pawn_keys{};
    std::array<std::array<uint64_t, 48>, 2> pawn_keys{};
    std::array<uint64_t, 16> castling_keys{};
    std::array<uint64_t, 8> en_passant_key{};
    uint64_t side_key;

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
        std::ranges::generate(en_passant_key.begin(), en_passant_key.end(), gen);
        side_key = dis(gnr);
    }
};

inline Zobrist zobrist_keys{};


