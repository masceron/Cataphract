#pragma once

#include "move.hpp"
#include "position.hpp"

struct MoveList
{
    Move list[256];
    Move* last;
    [[nodiscard]] Move* begin();
    [[nodiscard]] Move* end() const;
    [[nodiscard]] int64_t size() const;
    Move operator[](int index) const;
    MoveList();
    void reset();
};

enum class MoveType
{
    noisy, quiet, all
};

template <const MoveType type>
void pseudo_legals(const Position& cr_pos, MoveList& list);

void legals(const Position& cr_pos, MoveList& list);
