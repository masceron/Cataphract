cmake_minimum_required(VERSION 3.30)
project(CSEngine)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -Wextra -Wno-class-memaccess -Wno-implicit-fallthrough -static -pedantic -g -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -Wall -Wextra -Wno-class-memaccess -Wno-implicit-fallthrough -march=native -mtune=native -static -pedantic")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wextra -g -static -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "-march=native -mtune=native -O3 -static")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC not supported.")
endif()

add_executable(CSEngine main.cpp
        board/bitboard.hpp
        board/pieces/pawn.hpp
        board/pieces/knight.hpp
        board/pieces/king.hpp
        board/pieces/bishop.hpp
        board/pieces/rook.hpp
        board/pieces/slider.hpp
        board/pieces/slider.cpp
        position/fen.hpp
        position/position.hpp
        board/lines.hpp
        position/move.hpp
        position/zobrist.hpp
        position/movegen.hpp
        position/perft.hpp
        uci.hpp
        search/movepicker.hpp
        eval/eval.hpp
        search/search.hpp
        eval/transposition.hpp
        search/timer.hpp
        search/see.hpp
        position/position.cpp
        search/history.hpp
        eval/phase.hpp
        eval/nnue.hpp
        eval/incbin.h
        eval/accumulators.hpp
        eval/nnue.cpp
)
