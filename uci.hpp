#pragma once

#include <string>
#include <iostream>

#include "position/fen.hpp"
#include "position/perft.hpp"
#include "search/search.hpp"

namespace UCI
{
    inline void set_board(const std::string& fen)
    {
        if (fen.starts_with("startpos")) {
            fen_parse("startpos");
        }
        else if (fen.starts_with("fen ")) {
            if (fen_parse(fen.substr(4, std::string::npos)) == - 1) {
                std::cout << "Invalid FEN.\n";
            }
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    inline void perft(const std::string& depth)
    {
        try {
            divide(std::stoi(depth));
        } catch (...) {
            std::cout << "Unknown command.\n";
        }
    }

    inline void go_depth(const std::string& depth)
    {
        try {
            std::cout << "bestmove " + start_search(std::stoi(depth)) + "\n";
        } catch (...) {
            std::cout << "Unknown command.\n";
        }
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
            else if (input == "d") print_board();
            else if (input == "uci") {
                std::cout << "uciok\n";
            }
            else if (input.starts_with("position ")) set_board(input.substr(9, std::string::npos));
            else if (input.starts_with("go perft ")) perft(input.substr(9,std::string::npos));
            else if (input == "eval") std::cout << "Evaluation: " << eval(position) * (position.side_to_move == white ? 1 : -1) << "\n";
            else if (input != "quit") std::cout << "Unknown command.\n";
        }

    }

}
