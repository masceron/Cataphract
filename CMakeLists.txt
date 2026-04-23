cmake_minimum_required(VERSION 4.2.5)
project(Cataphract)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Cataphract main.cpp
        board/bitboard.hpp
        board/pieces/pawn.hpp
        board/pieces/knight.hpp
        board/pieces/king.hpp
        board/pieces/bishop.hpp
        board/pieces/rook.hpp
        board/pieces/slider.hpp
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
        eval/nnue.hpp
        eval/accumulators.hpp
        eval/nnue.cpp
        position/cuckoo.hpp
        search/time.hpp
        position/bench.hpp
        cataphract.hpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Cataphract PRIVATE "-lstdc++exp")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(Cataphract PRIVATE
                -O0 -Wall -Wextra -Wno-class-memaccess -g -march=native -fno-exceptions
        )

    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(Cataphract PRIVATE
                -O3 -fno-exceptions -flto=auto -fipa-pta -flto-partition=one -fwhole-program
                -Wall -Wextra -Wno-class-memaccess -march=native -mtune=native
        )
        target_compile_definitions(Cataphract PRIVATE NDEBUG)
        target_link_options(Cataphract PRIVATE
                -flto=auto -fwhole-program -static -s
        )
    endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(Cataphract PRIVATE
                -O0 -Wextra -fsanitize=undefined -g -march=native -fno-exceptions
        )
        target_link_options(Cataphract PRIVATE -fsanitize=undefined)

    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(Cataphract PRIVATE
                -O3 -flto=full -fno-exceptions -march=native -mtune=native
        )
        target_compile_definitions(Cataphract PRIVATE NDEBUG)
        target_link_options(Cataphract PRIVATE
                -flto=full -static
        )
    endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC not supported.")
endif ()

set(PGO "NONE" CACHE STRING "PGO Build Type: NONE, GENERATE, USE")

if (PGO STREQUAL "GENERATE")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(Cataphract PRIVATE -fprofile-generate)
        target_link_options(Cataphract PRIVATE -fprofile-generate)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(Cataphract PRIVATE -fprofile-instr-generate)
        target_link_options(Cataphract PRIVATE -fprofile-instr-generate)
    endif()
elseif (PGO STREQUAL "USE")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(Cataphract PRIVATE -fprofile-use -fprofile-correction)
        target_link_options(Cataphract PRIVATE -fprofile-use -fprofile-correction)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(Cataphract PRIVATE -fprofile-instr-use=default.profdata)
        target_link_options(Cataphract PRIVATE -fprofile-instr-use=default.profdata)
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(MERGE_PROFILES COMMAND llvm-profdata merge -output=default.profdata default.profraw)
else ()
    set(MERGE_PROFILES "")
endif ()