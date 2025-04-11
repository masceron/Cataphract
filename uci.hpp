#pragma once

#include <string>
#include <iostream>

#include "position/fen.hpp"
#include "position/perft.hpp"
#include "search/search.hpp"

namespace UCI
{
    inline void process_move(const std::string& move)
    {
        if (move.length() == 4) {
            MoveList moves;
            legals<all>(position, moves);
            const int from = algebraic_to_num(move.substr(0, 2));
            const int to = algebraic_to_num(move.substr(2, 2));
            if (from == -1 || to == -1) return;
            const auto _move = Move(from, to, 0);
            for (const auto& move_test : moves.list) {
                if ((move_test.move & 0xFFF) == _move.move) {
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
            MoveList moves;
            legals<all>(position, moves);
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

        if (moves_pos == std::string::npos) return;

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

    inline void perft(const std::string& depth)
    {
        try {
            divide(std::stoi(depth));
        } catch (...) {}
    }

    inline void go_depth(const std::string& depth)
    {
        try {
            start_search(std::stoi(depth));
        } catch (...) {}
    }

    inline void process()
    {
        fen_parse("startpos");
        std::string input;
        while (input != "quit") {
            std::getline(std::cin, input);
            const auto I = std::ranges::unique(input, [](auto lhs, auto rhs){ return lhs == rhs && lhs == ' '; } ).begin();
            input.erase(I, input.end());
            if (input.starts_with("go depth ")) go_depth(input.substr(9, std::string::npos));
            else if (input == "go infinite") go_depth("128");
            else if (input == "d") print_board();
            else if (input == "ucinewgame") TT::clear();
            else if (input == "uci") {
                std::cout << "uciok\n";
            }
            else if (input.starts_with("position ")) set_board(input.substr(9, std::string::npos));
            else if (input.starts_with("go perft ")) perft(input.substr(9,std::string::npos));
            else if (input == "eval") std::cout << "Evaluation: " << eval(position) * (position.side_to_move == white ? 1 : -1) << "\n";
            else if (input == "isready") std::cout << "readyok\n";
        }

    }

}
