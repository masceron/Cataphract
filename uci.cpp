#include <charconv>
#include <ranges>
#include <iostream>
#include <algorithm>
#include <print>

#include "engine.hpp"
#include "options.hpp"
#include "position/bench.hpp"
#include "position/perft.hpp"
#include "search/search.hpp"
#include "search/thread.hpp"
#include "search/timer.hpp"
#ifdef SPSA_TUNE
#include "search/params.hpp"
#endif
#include "uci.hpp"
#include "search/transposition.hpp"
#include "eval/nnue.hpp"

namespace UCI
{
    void perft(const std::string_view depth)
    {
        int depthTo = 0;
        std::from_chars(depth.data(), depth.data() + depth.size(), depthTo);
        if (depthTo < 1) return;
        divide(depthTo);
    }

    void set_option(std::string_view option)
    {
        auto tokens = option | std::views::split(' ');
        auto it = tokens.begin();
        ++it;

        if (const std::string_view name((*it).begin(), (*it).end()); name == "Hash")
        {
            ++it;
            ++it;
            const std::string_view value((*it).begin(), (*it).end());
            int32_t new_size = 0;
            std::from_chars(value.data(), value.data() + value.size(), new_size);

            if (new_size < 1 || new_size > 2048) return;
            TT::resize(static_cast<uint32_t>(new_size));
        }
        else if (name == "Clear")
        {
            ++it;
            if (std::string_view((*it).begin(), (*it).end()) == "Hash")
            {
                TT::clear(TT::table, TT::table_size);
            }
        }
        else if (name == "Threads")
        {
            ++it;
            ++it;
            const std::string_view value((*it).begin(), (*it).end());
            int thread = -1;
            std::from_chars(value.data(), value.data() + value.size(), thread);
            if (thread >= 1 && thread <= 1024) Options::threads = thread;
            ThreadPool::resize();
        }
        else if (name == "ShowCurrMove")
        {
            ++it;
            ++it;
            if (const std::string_view value((*it).begin(), (*it).end()); value == "true")
                Options::show_currmove = true;
            else if (value == "false")
                Options::show_currmove = false;
        }
        else if (name == "Move")
        {
            ++it;
            if (std::string_view((*it).begin(), (*it).end()) == "Overhead")
            {
                ++it;
                ++it;
                const std::string_view value((*it).begin(), (*it).end());
                int overhead = -1;
                std::from_chars(value.data(), value.data() + value.size(), overhead);
                if (overhead >= 0 && overhead <= 5000) Options::move_overhead = overhead;
            }
        }
#ifdef SPSA_TUNE
        else
        {
            ++it;
            if (it != tokens.end() && std::string_view((*it).begin(), (*it).end()) == "value")
            {
                ++it;
                if (it != tokens.end())
                {
                    const std::string_view value((*it).begin(), (*it).end());
                    Tuning::set_parameter(name, value);
                }
            }
        }
#endif
    }

    void bench(std::string_view args)
    {
        auto tokens = args | std::views::split(' ');
        auto it = tokens.begin();

        if (it != tokens.end()) ++it;

        int depth = 10;
        uint32_t tt_size = 16;
        if (it != tokens.end())
        {
            std::from_chars((*it).begin(), (*it).end(), depth);
            ++it;
        }
        if (it != tokens.end())
        {
            std::from_chars((*it).begin(), (*it).end(), tt_size);
        }
        run_bench(depth, tt_size);
    }

    void go(const std::string_view input)
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
            if (infinite) depth = MAX_PLY;

            search_thread = std::thread(start_search<false>,
                                         depth,
                                         movetime,
                                         wtime,
                                         btime,
                                         winc,
                                         binc,
                                         movestogo);

        }
    }

    void process()
    {
        set_board("startpos");

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
            if (!input.empty() && (input.back() == ' ' || input.back() == '\r')) input.pop_back();

            if (Timer::running)
            {
                if (input == "stop" && !Timer::is_search_cancelled)
                {
                    Timer::stop();
                }
                else if (input == "isready")
                {
                    std::println("readyok");
                }
                else if (input == "quit")
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
                if (search_thread.joinable()) search_thread.join();

                if (command == "position")
                {
                    set_board(input_view.substr(9));
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
                    new_game();
                }
                else if (command == "uci")
                {
                    std::println("id name Cataphract");
                    std::println("id author masceron\n");
                    std::println("option name Hash type spin default 64 min 1 max 2048");
                    std::println("option name Clear Hash type button");
                    std::println("option name Threads type spin default 1 min 1 max 1024");
                    std::println("option name ShowCurrMove type check default false");
                    std::println("option name Move Overhead type spin default 50 min 0 max 5000");
#ifdef SPSA_TUNE
                    Tuning::print_options();
#endif
                    std::println("uciok");
                    std::fflush(stdout);
                }
                else if (command == "eval")
                {
                    auto& position = ThreadPool::threads[0].position;
                    std::println("NNUE evaluation: {} (white side)",
                                 eval(position, ThreadPool::get(0).accumulator_stack) * (!position.side_to_move ? 1 : -1));
                }
                else if (command == "d")
                {
                    ThreadPool::get(0).position.print_board();
                }
                else if (command == "setoption")
                {
                    set_option(input_view.substr(10, std::string::npos));
                }
                else if (command == "bench")
                {
                    bench(input_view);
                }
#ifdef SPSA_TUNE
                else if (command == "spsa")
                {
                    Tuning::print_spsa_params();
                }
#endif
                else if (command == "quit")
                {
                    quit = true;
                }
            }
        }

        TT::free_tt();
        if (search_thread.joinable()) search_thread.join();
        ThreadPool::shutdown();
    }
}