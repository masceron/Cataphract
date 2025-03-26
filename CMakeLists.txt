cmake_minimum_required(VERSION 3.30)
project(CSEngine)
include_directories(./board, ./board/pieces, ./position)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

add_executable(CSEngine main.cpp
        board/bitboard.h
        board/pieces/pawn.h
        board/pieces/knight.h
        board/pieces/king.h
        board/pieces/bishop.h
        board/pieces/rook.h
        board/pieces/slider.h
        board/pieces/slider.cpp
        position/game.h
        position/fen.cpp
        position/fen.h
        position/position.h
        board/lines.h
        board/lines.cpp
)
