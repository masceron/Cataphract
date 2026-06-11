cmake_minimum_required(VERSION 4.2.5)
project(Cataphract)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_TUNING "Build with SPSA tuning hooks enabled" OFF)

add_executable(Cataphract main.cpp
        board/bitboard.hpp
        board/slider.hpp
        position/fen.hpp
        position/position.hpp
        board/lines.hpp
        position/move.hpp
        position/zobrist.hpp
        position/movegen.hpp
        position/perft.hpp
        uci.hpp
        search/movepicker.hpp
        search/search.hpp
        search/transposition.hpp
        search/timer.hpp
        search/see.hpp
        position/position.cpp
        search/history.hpp
        eval/nnue.hpp
        eval/accumulators.hpp
        position/cuckoo.hpp
        search/time.hpp
        position/bench.hpp
        Engine.hpp
        search/params.hpp
        search/params.cpp
        options.hpp
        eval/utils.hpp
        eval/arch.hpp
        eval/simd/simd.hpp
        search/thread.hpp
        eval/nnue.cpp
        search/search.cpp
        search/thread.cpp
        search/time.cpp
        search/timer.cpp
        search/see.cpp
        search/movepicker.cpp
        search/history.cpp
        position/fen.cpp
        position/movegen.cpp
        eval/accumulators.cpp
        search/transposition.cpp
        board/bitboard.cpp
        position/move.cpp
        Engine.cpp
        uci.cpp
        board/slider.cpp
        board/attacks.hpp
        position/cuckoo.cpp
        position/bench.cpp
        position/zobrist.cpp
        board/lines.cpp
        position/perft.cpp
        eval/utils.cpp
        eval/simd/avx512.cpp
        eval/simd/avx2.cpp
        eval/simd/scalar.cpp
)

if(ENABLE_TUNING)
    target_compile_definitions(Cataphract PRIVATE SPSA_TUNE)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Cataphract PRIVATE "-lstdc++exp")

    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(Cataphract PRIVATE
                -O0 -Wall -Wextra -Wno-class-memaccess -g -march=native -fno-omit-frame-pointer
        )

    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(Cataphract PRIVATE
                -Wall -Wextra -Wno-class-memaccess -DNDEBUG -fno-exceptions
                -ffunction-sections -fdata-sections -fno-rtti
                -O3 -flto=auto -fipa-pta
                -march=native -mtune=native
        )
        target_compile_definitions(Cataphract PRIVATE NDEBUG)
        target_link_options(Cataphract PRIVATE
                -flto=auto -static -s
                -Wl,--gc-sections
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(Cataphract PRIVATE -O0 -Wextra -g -march=native -fno-exceptions
                -fno-omit-frame-pointer
                -fsanitize=undefined -fsanitize=implicit-integer-truncation
                -fsanitize=implicit-integer-arithmetic-value-change -fsanitize=implicit-integer-conversion
                -fsanitize=implicit-conversion -fsanitize=signed-integer-overflow
                -fsanitize=nullability -fsanitize=local-bounds
                -fsanitize=float-divide-by-zero -fsanitize=integer-divide-by-zero
                -fsanitize=implicit-unsigned-integer-truncation -fsanitize=implicit-signed-integer-truncation
                -fsanitize=implicit-integer-sign-change
                -fsanitize=thread
                -fno-sanitize=shift
                -Wno-c23-extensions
        )
        target_link_options(Cataphract PRIVATE
                -fsanitize=undefined -fsanitize=implicit-integer-truncation
                -fsanitize=implicit-integer-arithmetic-value-change -fsanitize=implicit-integer-conversion
                -fsanitize=implicit-conversion -fsanitize=signed-integer-overflow
                -fsanitize=nullability -fsanitize=local-bounds
                -fsanitize=float-divide-by-zero -fsanitize=integer-divide-by-zero
                -fsanitize=implicit-unsigned-integer-truncation -fsanitize=implicit-signed-integer-truncation
                -fsanitize=implicit-integer-sign-change
                -fsanitize=thread
                -fno-sanitize=shift
                -fuse-ld=lld
        )

    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(Cataphract PRIVATE
                -Wall -Wextra -DNDEBUG
                -O3 -flto=full -fno-exceptions -march=native -mtune=native -fomit-frame-pointer
                -ffunction-sections -fdata-sections -fexperimental-library -fno-rtti
                -Wno-c23-extensions
        )
        target_compile_definitions(Cataphract PRIVATE NDEBUG)
        target_link_options(Cataphract PRIVATE
                -flto=full -static -s
                -fuse-ld=lld
                -Wl,--gc-sections -Wl,--icf=all
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC not supported.")
endif ()