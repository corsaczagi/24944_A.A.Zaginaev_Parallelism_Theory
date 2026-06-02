cmake_minimum_required(VERSION 3.18)

project(HeatEquationSolver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS program_options)

#
# HOST
#
add_executable(heat_host main.cpp)

target_compile_options(
    heat_host
    PRIVATE
    -O3
    -acc=host
    -Minfo=all
)

target_link_libraries(
    heat_host
    PRIVATE
    Boost::program_options
)

#
# MULTICORE
#
add_executable(heat_multicore main.cpp)

target_compile_options(
    heat_multicore
    PRIVATE
    -O3
    -acc=multicore
    -Minfo=all
)

target_link_libraries(
    heat_multicore
    PRIVATE
    Boost::program_options
)

#
# GPU
#
add_executable(heat_gpu main.cpp)

target_compile_options(
    heat_gpu
    PRIVATE
    -O3
    -acc=gpu
    -Minfo=all
)

target_link_libraries(
    heat_gpu
    PRIVATE
    Boost::program_options
)

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")