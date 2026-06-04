#pragma once
#include "board/lines.hpp"
#include "eval/nnue.hpp"
#include "eval/transposition.hpp"
#include "position/cuckoo.hpp"
#include "position/fen.hpp"
#include "position/movegen.hpp"
#include "position/position.hpp"
#include "position/zobrist.hpp"
#include "search/params.hpp"
#include "search/thread.hpp"

namespace Cataphract
{
    inline void new_game()
    {
        TT::current_generation = 8;
        ThreadPool::start_workers(WorkerTask::NewGame);
        ThreadPool::get(0).new_game();
        ThreadPool::wait_for_workers();
    }

    inline void start()
    {
        reduction_cal();
        prune_cal();
        Zobrist::generate_keys();
        generate_lines();
        Cuckoo::init();
        TT::alloc();

        std::println("Cataphract v1.6 by masceron");
        std::fflush(stdout);
    }

    inline void process_move(Position& position, const std::string_view move, MoveList& list)
    {
        if (move.length() == 4)
        {
            const int from = algebraic_to_num(move.substr(0, 2));
            const int to = algebraic_to_num(move.substr(2, 2));
            if (from == -1 || to == -1) return;
            const auto _move = Move(from, to, MoveFlag::quiet_move);
            legals<MoveType::all>(position, list);
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
            const auto _move = Move(from, to, static_cast<MoveFlag>(flag));

            legals<MoveType::all>(position, list);

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
        auto& position = ThreadPool::get(0).position;
        auto& accumulators = ThreadPool::get(0).accumulator_stack;

        if (fen.starts_with("startpos"))
        {
            fen_parse(position, "startpos");
        }
        else if (fen.starts_with("fen "))
        {
            if (fen_parse(position, fen.substr(4, moves_pos == std::string::npos ? std::string::npos : moves_pos - 5)) == -1)
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
                process_move(position, std::string_view{move}, list);
            }
        }
        TT::advance();

        ThreadPool::setup();

        ThreadPool::start_workers(WorkerTask::Refresh);
        NNUE::refresh_accumulators(position, accumulators);

        ThreadPool::wait_for_workers();
    }
}
