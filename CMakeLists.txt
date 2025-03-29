cmake_minimum_required(VERSION 3.30)
project(CSEngine)
include_directories(./board, ./board/pieces, ./position)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -Wall -Wextra -Werror -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/W4 /wd4244 /wd4146 /std:c++latest")
    set(CMAKE_CXX_FLAGS_RELEASE "/W4 /wd4244 /wd4146 /O2 /favor:AMD64 /GA /std:c++latest")
endif()


add_executable(CSEngine main.cpp
        board/bitboard.h
        board/pieces/pawn.h
        board/pieces/knight.h
        board/pieces/king.h
        board/pieces/bishop.h
        board/pieces/rook.h
        board/pieces/slider.h
        board/pieces/slider.cpp
        position/fen.cpp
        position/fen.h
        position/position.h
        board/lines.h
        board/lines.cpp
        position/move.h
        position/zobrist.h
)
