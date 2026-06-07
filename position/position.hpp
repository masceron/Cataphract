#pragma once

#include <cstdint>
#include <array>
#include <string>

enum Piece : uint8_t;
struct Move;
struct AccumulatorEntry;

bool color_of(Piece piece);

struct State
{
    uint64_t pawn_key;
    std::array<uint64_t, 2> non_pawn_keys;
    uint64_t major_key;
    uint16_t ply;
    uint8_t castling_rights;
    uint8_t rule_50;
    int8_t en_passant_square;

    uint64_t key = 0;
    uint64_t pinned;
    uint64_t checker;
    uint64_t check_blocker;
    uint64_t attacks;
    State* previous;
    Piece captured_piece;
    int8_t repetition;
};

struct Position
{
    std::array<Piece, 64> piece_on;
    std::array<uint64_t, 14> boards;
    std::array<uint64_t, 3> occupations;
    bool side_to_move;
    State* state = nullptr;
    Position();
    void new_game();

    void move_piece(int from, int to);
    void put_piece(Piece piece, int sq);
    void remove_piece(int sq);

    void make_move(Move move, State &st);
    void unmake_move(const Move &move);
    void do_move(Move move);

    void make_null_move(State& st);
    void unmake_null_move();
    void fill_info() const;
    void get_pinned() const;
    void get_attacks() const;
    void get_checks() const;

    [[nodiscard]] bool upcoming_repetition(int ply) const;

    template <const bool side>
    [[nodiscard]] uint64_t get_pinned_board_of() const;

    template<const bool side>
    [[nodiscard]] uint64_t get_checker_of() const;

    template<const bool side>
    [[nodiscard]] uint64_t get_check_blocker_of() const;

    template<const bool side>
    [[nodiscard]] uint64_t get_attacked_map_of() const;

    [[nodiscard]] bool is_legal(Move move) const;
    [[nodiscard]] bool is_pseudo_legal(Move move) const;
    [[nodiscard]] bool is_quiet(Move move) const;

    void construct_zobrist_key() const;

    void print_board() const;

    [[nodiscard]] std::string to_fen() const;
};
