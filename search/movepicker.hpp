#pragma once

#include "movegen.hpp"
#include "move.hpp"

struct MovePicker
{
    MoveList move_list;
    uint8_t offset = 0;
    uint8_t size;
    explicit MovePicker(Position& _pos): move_list(legal_move_generator(_pos))
    {
        size = move_list.size();
    }
    Move pick()
    {
        if (offset == size) return move_none;
        return move_list.list[offset++];
    }

    void set_principal(const Move& move)
    {
        for (int i = 0; i < move_list.size(); i++) {
            if (move_list.list[i] == move) {
                const auto& tmp = move_list.list[0];
                move_list.list[0] = move;
                move_list.list[i] = tmp;
                return;
            }
        }
    }
};
