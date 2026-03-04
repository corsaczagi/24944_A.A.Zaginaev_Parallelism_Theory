# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(MatrixVectorMult)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(matrix_vector_mult main-1.cpp)
target_link_libraries(matrix_vector_mult pthread)