message(STATUS " Starting PGO Build ")

message(STATUS "\nConfiguring for PGO Generation...")
execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DPGO=GENERATE . RESULT_VARIABLE res)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "Configuration failed!")
endif()

message(STATUS "\nBuilding instrumented executable...")
execute_process(COMMAND ${CMAKE_COMMAND} --build . -j RESULT_VARIABLE res)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "Build failed!")
endif()

message(STATUS "\nRunning benchmark...")

if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Cataphract.exe")
    set(ENGINE_EXEC "./Cataphract.exe")
else()
    set(ENGINE_EXEC "./Cataphract")
endif()

execute_process(COMMAND ${ENGINE_EXEC} bench RESULT_VARIABLE res)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "Benchmark failed!")
endif()

if(EXISTS "default.profraw")
    message(STATUS "Merging Clang profraw data...")
    execute_process(COMMAND llvm-profdata merge -output=default.profdata default.profraw)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DPGO=USE . RESULT_VARIABLE res)

message(STATUS "Building final optimized executable...")
execute_process(COMMAND ${CMAKE_COMMAND} --build . -j RESULT_VARIABLE res)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "Final build failed!")
endif()