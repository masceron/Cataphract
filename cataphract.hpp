#pragma once
#include "board/lines.hpp"
#include "eval/nnue.hpp"
#include "eval/transposition.hpp"
#include "position/cuckoo.hpp"
#include "position/fen.hpp"
#include "position/movegen.hpp"
#include "position/position.hpp"
#include "position/zobrist.hpp"
#include "search/history.hpp"

namespace Cataphract
{
    inline void new_game()
    {
        TT::clear();
        ButterflyHistory::clear();
        PieceToHistory::clear();
        Killers::clear();
        Capture::clear();
        Corrections::clear();
        Continuation::clear();
    }

    inline void start()
    {
        Zobrist::generate_keys();
        generate_lines();
        Cuckoo::init();
        TT::alloc();

        std::println("Cataphract v1.4 by masceron");
        std::fflush(stdout);
    }

    inline void process_move(const std::string_view move, MoveList& list)
    {
        if (move.length() == 4)
        {
            const int from = algebraic_to_num(move.substr(0, 2));
            const int to = algebraic_to_num(move.substr(2, 2));
            if (from == -1 || to == -1) return;
            const auto _move = Move(from, to, 0);
            legals<all>(position, list);
            for (const auto& move_test : list)
            {
                if ((move_test.move & 0xFFF) == _move.move)
                {
                    position.do_move(move_test);
                    return;
                }
            }
        }
        else if (move.length() == 5)
        {
            const int from = algebraic_to_num(move.substr(0, 2));
            const int to = algebraic_to_num(move.substr(2, 2));
            int flag = 0;
            if (from == -1 || to == -1) return;
            switch (move[4])
            {
            case 'n':
                flag = 8;
                break;
            case 'b':
                flag = 9;
                break;
            case 'r':
                flag = 10;
                break;
            case 'q':
                flag = 11;
                break;
            default:
                return;
            }
            flag += abs(from - to) % 8 == 0 ? 0 : 4;
            const auto _move = Move(from, to, flag);

            legals<all>(position, list);

            for (const auto& move_test : list)
            {
                if (move_test.move == _move.move)
                {
                    position.do_move(move_test);
                    return;
                }
            }
        }
    }

    inline void set_board(const std::string_view fen)
    {
        const auto moves_pos = fen.find("moves ");

        if (fen.starts_with("startpos"))
        {
            fen_parse("startpos");
        }
        else if (fen.starts_with("fen "))
        {
            if (fen_parse(fen.substr(4, moves_pos == std::string::npos ? std::string::npos : moves_pos - 5)) == -1)
            {
                return;
            }
        }
        else return;

        if (moves_pos != std::string::npos)
        {
            MoveList list;
            for (auto moves = fen.substr(moves_pos + 6, std::string::npos) | std::views::split(' ');
                 auto move : moves)
            {
                list.reset();
                process_move(std::string_view{move}, list);
            }
        }
        TT::advance();
        NNUE::refresh_accumulators(position);
    }
}
