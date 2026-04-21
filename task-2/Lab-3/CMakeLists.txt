cmake_minimum_required(VERSION 3.10)
project(SLAU_OpenMP)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

add_executable(slau main.cpp)
target_link_libraries(slau OpenMP::OpenMP_CXX)

# Для оптимизации
target_compile_options(slau PRIVATE -O3 -march=native)