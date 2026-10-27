cmake_minimum_required(VERSION 3.15)
project(SkipListChallenge C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/results)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/include)

add_executable(test_main src/skiplist.c tests/test_main.c)

target_compile_definitions(test_main PRIVATE SKIPLIST_COUNT_WORK)

add_executable(benchmark_runner src/skiplist.c src/benchmark_driver.cpp)

if(MSVC)
    target_compile_definitions(benchmark_runner PRIVATE BENCHMARK_STATIC_DEFINE)
    
    set(BM_LIB "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/build/src/Release/benchmark.lib")
    set(BM_MAIN_LIB "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/build/src/Release/benchmark_main.lib")
    
    target_link_libraries(benchmark_runner PRIVATE ${BM_LIB} ${BM_MAIN_LIB} shlwapi)
else()
    # Paths for Linux / GCC
    set(BM_LIB "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/build/src/libbenchmark.a")
    set(BM_MAIN_LIB "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/build/src/libbenchmark_main.a")
    
    target_link_libraries(benchmark_runner PRIVATE ${BM_LIB} ${BM_MAIN_LIB} pthread)
endif()