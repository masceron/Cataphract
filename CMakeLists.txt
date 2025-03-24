cmake_minimum_required(VERSION 3.30)
project(CSEngine)
include_directories(./board)
set(CMAKE_CXX_STANDARD 23)

add_executable(CSEngine main.cpp
        board/bitboard.cpp
        board/bitboard.h)
