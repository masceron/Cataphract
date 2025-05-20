#pragma once

#include <string>
#include <iostream>
#include <thread>
#include <sstream>
#include <ranges>
#include <algorithm>


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

    // Removed go_depth and go_time as their logic is merged into go and start_search

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



inline void start_search(int depth_param, int movetime_param, int wtime_param, int btime_param, int winc_param, int binc_param, int movestogo_param)
{
    int allocated_time_ms = -1;
    int search_depth = 127; // Default to infinite depth

    if (movetime_param > 0) {
        // Fixed time per move
        allocated_time_ms = movetime_param;
        search_depth = 127;
    } else if (depth_param > 0 && depth_param < 127) {
        // Fixed depth search
        search_depth = depth_param;
        allocated_time_ms = -1; // No time limit for fixed depth
    } else if (wtime_param > 0 || btime_param > 0) {
        // Time control based on remaining time and increment
        int remaining_time = (position.side_to_move == white) ? wtime_param : btime_param;
        int increment = (position.side_to_move == white) ? winc_param : binc_param;

        if (movestogo_param > 0) {
            // Use movestogo if provided
            allocated_time_ms = remaining_time / movestogo_param + increment;
        } else {
            // Fallback heuristic if movestogo is not provided
            allocated_time_ms = remaining_time / 30 + increment; // Simple division heuristic
        }

        // Ensure a minimum time allocation
        allocated_time_ms = std::max(allocated_time_ms, 100); // Minimum 100ms

        search_depth = 127; // Infinite depth within the time limit
    } else {
        // No specific limits provided (e.g., 'go infinite' or just 'go')
        // Default to infinite depth and a very large time limit
        search_depth = 127;
        allocated_time_ms = 6000000; // 100 minutes as a practical infinite limit
    }

    // Start the timer if a time limit was determined
    if (allocated_time_ms > 0) {
        Timer::start(allocated_time_ms);
    } else {
        // If no time limit (fixed depth or true infinite), use a very large timer
        // This timer is mostly to allow 'stop' command to work, not for actual time control
        Timer::start(6000000); // 100 minutes
    }

    node_searched = 0;
    Move best_move = move_none;
    std::list<Move> principal_variation;

    // Assuming search_stack and accumulator_stack setup is correct
    std::array<SearchEntry, 140> search_stack;
    search_stack_init(search_stack.data());

    accumulator_stack.clear();
    accumulator_stack.emplace_back(root_accumulators, position);

    int alpha = negative_infinity;
    int beta = infinity;

    int window = pawn_weight / 2; // Initial aspiration window

    // Iterative deepening loop
    for (int cr_depth = 1; cr_depth <= search_depth; cr_depth++) {
        if (is_search_cancelled) break; // Stop if timer expired or 'stop' received

        seldepth = 0;
        int score;

        // Aspiration window loop
        while (true) {

            score = search(position, alpha, beta, cr_depth, principal_variation, false, search_stack.data() + 4);

            if (is_search_cancelled) break;


            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(score - window, static_cast<int>(negative_infinity));
            }
            else if (score >= beta) {
                beta = std::min(score + window, static_cast<int>(infinity));
            }
            else {

                break;
            }
            window += window / 2;
        }

        if (is_search_cancelled) break;

        // Output UCI info string
        const double elapsed = Timer::elapsed();
        std::cout << "info depth " << cr_depth << " seldepth " << seldepth << " score";

        if (score < -mate_in_max_ply) std::cout << " mate " << - std::ceil((mate_value + score) / 2.0);
        else if (score > mate_in_max_ply) std::cout << " mate " << std::ceil((mate_value - score) / 2.0);
        else std::cout << " cp " << score;

        std::cout << " nodes " << node_searched
        << " nps " << std::fixed << std::setprecision(0) << node_searched / elapsed * 1000000
        << " hashfull " << TT::full()
        << " time " << elapsed / 1000
        << " pv ";

        for (auto x : principal_variation) {
            std::cout << x.get_move_string() << " ";
        }
        std::cout << std::endl;

        // Update best_move from the principal variation
        if (!principal_variation.empty()) {
             best_move = principal_variation.front();
        }

        // Optional: Stop search early if depth limit reached and not in time control mode
        if (depth_param > 0 && cr_depth >= depth_param) {
             break;
        }
    }

    // Stop the timer and join the thread
    Timer::stop();
    if (Timer::timer_thread.joinable()) {
        Timer::timer_thread.join();
    }

    // If no best move was found (e.g., search cancelled very early), pick a legal move
    if (best_move == move_none) {
        MoveList legal_moves = legals<all>(position); // Assuming legals function exists

        // Check if there are any legal moves
        if ((legal_moves.last - legal_moves.begin()) > 0) {
             best_move = legal_moves.list[0]; // Pick the first legal move
        } else {
            // Handle case with no legal moves (checkmate or stalemate)
            // The engine should ideally report checkmate/stalemate here
            // For now, keeping it as per original structure, which might imply
            // the engine just outputs "bestmove (none)" or similar depending on
            // how move_none is handled downstream. A better approach would be
            // to signal checkmate/stalemate via UCI.
        }
    }

    // Output the final best move
    std::cout << "bestmove " << best_move.get_move_string() << std::endl;
}



    inline void go(std::string& input)
    {
        std::stringstream ss(input);
        std::string token;
        ss >> token; // Consume "go" token

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

        // Parse go parameters
        while (ss >> token) {
            if (token == "depth") {
                ss >> depth;
            } else if (token == "movetime") {
                ss >> movetime;
            } else if (token == "wtime") {
                ss >> wtime;
            } else if (token == "btime") {
                ss >> btime;
            } else if (token == "winc") {
                ss >> winc;
            } else if (token == "binc") {
                ss >> binc;
            } else if (token == "movestogo") {
                ss >> movestogo;
            } else if (token == "infinite") {
                infinite = true;
            } else if (token == "perft") {
                perft_cmd = true;
                ss >> perft_depth;
            }
            // Ignore other parameters like "searchmoves", "ponder", "nodes", "mate" for now
        }
        // Handle perft command separately after parsing all parameters
         if (perft_cmd) {
         // Call perft with the parsed depth
           perft(std::to_string(perft_depth));
        } else {

            if (infinite) depth = 127; // Use 127 as a marker for infinite depth

            search_thread = std::thread(start_search, depth, movetime, wtime, btime, winc, binc, movestogo);
            search_thread.detach();
        }
    }

    inline void process()
    {


        fen_parse("startpos"); // Set initial position
        NNUE::refresh_accumulators(&root_accumulators, position); // Initialize NNUE state

        std::string input;
        bool quit = false;
        while (!quit) {
            std::getline(std::cin, input);

            // Clean up multiple spaces and leading/trailing spaces
            auto I = std::ranges::unique(input, [](auto lhs, auto rhs){ return lhs == rhs && lhs == ' '; } ).begin();
            input.erase(I, input.end());
            if (!input.empty() && input.front() == ' ') input.erase(0, 1);
            if (!input.empty() && input.back() == ' ') input.pop_back();

            // Handle 'stop' command while search is running
            if (Timer::running) {
                if (input == "stop" && !is_search_cancelled) {
                    Timer::stop();
                }
                // Ignore other commands while searching, except potentially 'quit'
                if (input == "quit") {
                    Timer::stop(); // Signal stop to the search thread
                    if (search_thread.joinable()) {
                        search_thread.join(); // Wait for search thread to finish
                    }
                    quit = true;
                }
            }
            else {
                // Process commands when not searching
                std::string command = input.substr(0, input.find(' '));
                if (command.empty() && !input.empty()) command = input; // Handle commands with no parameters

                if (command == "position") {

                    set_board(input.substr(9, std::string::npos));
                    NNUE::refresh_accumulators(&root_accumulators, position); // Update NNUE state for new position
                }
                else if (command == "go") {
                    go(input); // Start search
                }
                else if (command == "isready") {
                    std::cout << "readyok" << std::endl;
                }
                else if (command == "ucinewgame") {
                    new_game(); // Reset engine state
                    fen_parse("startpos"); // Reset board to startpos
                    NNUE::refresh_accumulators(&root_accumulators, position); // Reset NNUE state
                }
                else if (command == "uci") {
                    std::cout << "id name Cataphract" << "\n" <<"id author masceron\n\n"
                    << "option name Hash type spin default 256 min 1 max 1024\n"
                    << "option name Clear Hash type button\n"
                    // UCI options for time controls are not typically listed here,
                    // as wtime/btime/winc/binc/movestogo are parameters to 'go', not options.
                    << "uciok" << std::endl;
                }
                else if (command == "eval") {

                    std::cout << "NNUE evaluation: "
                    << NNUE::evaluate(position, &root_accumulators) * (position.side_to_move == white ? 1 : -1)
                    << " (white side)" << std::endl;
                }
                else if (command == "d") {

                    position.print_board();
                }
                else if (command == "setoption") {

                    set_option(input.substr(15, std::string::npos));
                }
                else if (command == "quit") {
                    quit = true; // Exit the loop
                }
                // Add handling for other potential commands if needed
            }
        }

        // Clean up resources before exiting
        TT::free_tt(); // Assuming TT::free_tt exists
    }

}