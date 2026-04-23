#pragma once

#include <string>
#include <thread>
#include <sstream>
#include <ranges>
#include <algorithm>
#include <charconv>
#include <iostream>
#include <print>

#include "cataphract.hpp"
#include "position/bench.hpp"
#include "position/perft.hpp"
#include "search/search.hpp"

namespace UCI
{
    static std::jthread search_thread;

    inline void perft(const std::string_view depth)
    {
        int depthTo = 0;
        std::from_chars(depth.data(), depth.data() + depth.size(), depthTo);
        if (depthTo < 1) return;
        divide(depthTo);
    }

    inline void set_option(std::string_view option)
    {
        const auto value_index = option.find(" value ");

        if (const std::string_view name = option.substr(0, value_index); name == "Hash")
        {
            const std::string_view value = option.substr(value_index + 7);
            int32_t new_size = 0;
            std::from_chars(value.data(), value.data() + value.size(), new_size);

            if (new_size < 1 || new_size > 2048) return;
            TT::resize(static_cast<uint32_t>(new_size));
        }
        else if (name == "Clear Hash")
        {
            TT::clear();
        }
    }

    inline void bench(std::string_view args)
    {
        auto tokens = args | std::views::split(' ');
        auto it = tokens.begin();

        if (it != tokens.end()) ++it;

        int depth = 15;
        uint32_t tt_size = 256;
        if (it != tokens.end())
        {
            std::from_chars((*it).begin(), (*it).end(), depth);
            ++it;
        }
        if (it != tokens.end())
        {
            std::from_chars((*it).begin(), (*it).end(), tt_size);
        }
        Bench::run(depth, tt_size);
    }

    inline void go(const std::string_view input)
    {
        auto tokens = input | std::views::split(' ');
        auto it = tokens.begin();

        int depth = 0;
        int movetime = 0;
        int wtime = 0;
        int btime = 0;
        int winc = 0;
        int binc = 0;
        int movestogo = 0;
        uint32_t nodes = UINT32_MAX;
        bool infinite = false;
        bool perft_cmd = false;
        int perft_depth = 0;

        if (it != tokens.end()) ++it;

        while (it != tokens.end())
        {
            if (const std::string_view token{(*it).begin(), (*it).end()}; token == "depth")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), depth);
            }
            else if (token == "movetime")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), movetime);
            }
            else if (token == "nodes")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), nodes);
            }
            else if (token == "wtime")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), wtime);
            }
            else if (token == "btime")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), btime);
            }
            else if (token == "winc")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), winc);
            }
            else if (token == "binc")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), binc);
            }
            else if (token == "movestogo")
            {
                ++it;
                std::from_chars((*it).begin(), (*it).end(), movestogo);
            }
            else if (token == "infinite")
            {
                infinite = true;
            }
            else if (token == "perft")
            {
                perft_cmd = true;
                ++it;
                std::from_chars((*it).begin(), (*it).end(), perft_depth);
            }
            ++it;
        }

        if (perft_cmd)
        {
            perft(std::to_string(perft_depth));
        }
        else
        {
            if (infinite) depth = 127;

            search_thread = std::jthread(start_search<false>,
                                         depth,
                                         movetime,
                                         wtime,
                                         btime,
                                         winc,
                                         binc,
                                         movestogo,
                                         nodes);
        }
    }

    inline void process()
    {
        Cataphract::set_board("startpos");

        std::string input;
        bool quit = false;
        while (!quit)
        {
            std::getline(std::cin, input);

            auto I = std::ranges::unique(input, [](auto lhs, auto rhs)
            {
                return lhs == rhs && lhs == ' ';
            }).begin();

            input.erase(I, input.end());

            if (!input.empty() && input.front() == ' ') input.erase(0, 1);
            if (!input.empty() && input.back() == ' ') input.pop_back();

            if (Timer::running)
            {
                if (input == "stop" && !Timer::is_search_cancelled)
                {
                    Timer::stop();
                }
                if (input == "quit")
                {
                    Timer::stop();
                    quit = true;
                }
            }
            else
            {
                std::string_view input_view(input);
                std::string_view command = input_view.substr(0, input_view.find(' '));

                if (command.empty() && !input.empty()) command = input_view;

                if (command == "position")
                {
                    Cataphract::set_board(input_view.substr(9));
                }
                else if (command == "go")
                {
                    go(input);
                }
                else if (command == "isready")
                {
                    std::println("readyok");
                }
                else if (command == "ucinewgame")
                {
                    Cataphract::new_game();
                }
                else if (command == "uci")
                {
                    std::println("id name Cataphract");
                    std::println("id author masceron\n");
                    std::println("option name Hash type spin default 256 min 1 max 2048");
                    std::println("option name Clear Hash type button");
                    std::println("uciok");
                    std::fflush(stdout);
                }
                else if (command == "eval")
                {
                    std::println("NNUE evaluation: {} (white side)",
                                 eval(position) * (!position.side_to_move ? 1 : -1));
                }
                else if (command == "d")
                {
                    position.print_board();
                }
                else if (command == "setoption")
                {
                    set_option(input_view.substr(10, std::string::npos));
                }
                else if (command == "bench")
                {
                    bench(input_view);
                }
                else if (command == "quit")
                {
                    quit = true;
                }
            }
        }

        TT::free_tt();
    }
}
