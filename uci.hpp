#pragma once

#include <string>
#include <iostream>
#include <thread>

#include "eval/nnue.hpp"
#include "position/cuckoo.hpp"
#include "position/fen.hpp"
#include "position/perft.hpp"
#include "search/search.hpp"

namespace UCI
{
    static std::thread search_thread;
    inline void new_game()
    {
        TT::clear();
        History::clear();
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
        accumulator_stack.reserve(129);

        std::cout << "Cataphract v1.0 by masceron" << std::endl;
    }
    inline void process_move(const std::string& move)
    {
        if (move.length() == 4) {
            const MoveList moves = legals<all>(position);
            const int from = algebraic_to_num(move.substr(0, 2));
            const int to = algebraic_to_num(move.substr(2, 2));
            if (from == -1 || to == -1) return;
            const auto _move = Move(from, to, 0);
            for (int i = 0; i < moves.size(); i++) {
                if (const auto move_test = moves[i]; (move_test.move & 0xFFF) == _move.move) {
                    position.do_move(move_test);
                    return;
                }
            }
        }
        else if (move.length() == 5) {
            const int from = algebraic_to_num(move.substr(0, 2));
            const int to = algebraic_to_num(move.substr(2, 2));
            int flag = 0;
            if (from == -1 || to == -1) return;
            switch (move[4]) {
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
            flag += (abs(from - to) % 8 == 0) ? 0 : 4;
            const MoveList moves = legals<all>(position);
            const auto _move = Move(from, to, flag);
            for (const auto& move_test : moves.list) {
                if ((move_test.move) == _move.move) {
                    position.do_move(move_test);
                    return;
                }
            }
        }
    }

    inline void set_board(const std::string& fen)
    {
        const auto moves_pos = fen.find("moves ");

        if (fen.starts_with("startpos")) {
            fen_parse("startpos");
        }
        else if (fen.starts_with("fen ")) {
            if (moves_pos == std::string::npos) {
                if (fen_parse(fen.substr(4, std::string::npos)) == - 1) {
                    return;
                }
            }
            else if (fen_parse(fen.substr(4, moves_pos - 5)) == - 1) {
                return;
            }
        }
        else {
            return;
        }

        if (moves_pos != std::string::npos) {
            std::string moves = fen.substr(moves_pos + 6, std::string::npos);
            std::ranges::transform(moves, moves.begin(), [](const unsigned char c){ return std::tolower(c); });

            auto limiter = moves.find(' ');
            size_t begin = 0;
            while (begin != std::string::npos) {
                process_move(moves.substr(begin, limiter - begin));
                if (limiter != std::string::npos) {
                    begin = limiter + 1;
                    limiter = moves.find(' ', begin);
                }
                else begin = std::string::npos;
            }
        }
        TT::age();
        NNUE::refresh_accumulators(&root_accumulators, position);
    }

    inline void perft(const std::string& depth)
    {
        try {
            divide(std::stoi(depth));
        } catch (...) {}
    }

    inline void go_depth(const std::string& depth)
    {
        try {
            search_thread = std::thread(start_search, std::stoi(depth), -1);
            search_thread.detach();
        } catch (...) {}
    }

    inline void go_time(const std::string& time_num)
    {
        try {
            search_thread = std::thread(start_search, 0, std::stoi(time_num));
            search_thread.detach();
        }
        catch (...) {}
    }

   inline void set_option(const std::string& option)
    {
        const int value_index = option.find(" value ");
        const std::string name = option.substr(0, value_index);

        if (name == "Hash") {
            const std::string value = option.substr(value_index + 7, std::string::npos);
            try {
                const int32_t new_size = std::stoi(value);
                if (new_size < 1 || new_size > 1024) return;
                TT::resize(static_cast<uint32_t>(new_size));
            } catch (...) {}
        }
        else if (name == "Clear Hash") {
            TT::clear();
        }
    }

    inline void go(std::string& input)
    {
        auto cutoff = input.find(' ', 3);
        if (cutoff != std::string::npos) cutoff -= 3;
        const std::string command = input.substr(3, cutoff);

        if (command == "movetime") go_time(input.substr(12, std::string::npos));
        else if (command == "depth") go_depth(input.substr(9, std::string::npos));
        else if (command == "perft") perft(input.substr(9,std::string::npos));
        else if (command == "infinite") go_depth("127");
    }

    inline void process()
    {
        fen_parse("startpos");
        NNUE::refresh_accumulators(&root_accumulators, position);
        std::string input;
        bool quit = false;
        while (!quit) {
            std::getline(std::cin, input);
            const auto I = std::ranges::unique(input, [](auto lhs, auto rhs){ return lhs == rhs && lhs == ' '; } ).begin();
            input.erase(I, input.end());
            if (Timer::running) {
                if (input == "stop" && !is_search_cancelled) Timer::stop();
            }
            else {
                std::string command = input.substr(0, input.find(' '));

                if (command == "position") set_board(input.substr(9, std::string::npos));
                else if (command == "go") go(input);
                else if (command == "isready") std::cout << "readyok" << std::endl;
                else if (command == "ucinewgame") new_game();
                else if (command == "uci") {
                    std::cout << "id name Cataphract" << "\n" <<"id author masceron\n\n"
                    << "option name Hash type spin default 256 min 1 max 1024\n"
                    << "option name Clear Hash type button\n"
                    << "uciok" << std::endl;
                }
                else if (command == "eval") {
                    std::cout << "NNUE evaluation: "
                    << NNUE::evaluate(position, &root_accumulators) * (position.side_to_move == white ? 1 : -1)
                    << " (white side)" << std::endl;
                }
                else if (command == "d") position.print_board();
                else if (command == "setoption") set_option(input.substr(15, std::string::npos));
                else if (command == "quit") quit = true;
            }
        }

        TT::free_tt();
    }

}
