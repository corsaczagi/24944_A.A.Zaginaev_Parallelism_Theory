cmake_minimum_required(VERSION 3.10)
project(IntegrateOMP)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenMP REQUIRED)

add_executable(integrate main.cpp)
target_link_libraries(integrate OpenMP::OpenMP_CXX)