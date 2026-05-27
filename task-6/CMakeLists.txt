cmake_minimum_required(VERSION 3.16)

project(jacobi_openacc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS program_options)

# HOST
add_executable(jacobi_host jacobi.cpp)

target_compile_options(jacobi_host PRIVATE
    -acc=host
    -Minfo=all
    -O2
)

target_link_libraries(jacobi_host
    Boost::program_options
)

# MULTICORE
add_executable(jacobi_multi jacobi.cpp)

target_compile_options(jacobi_multi PRIVATE
    -acc=multicore
    -Minfo=all
    -O2
)

target_link_libraries(jacobi_multi
    Boost::program_options
)

# GPU
add_executable(jacobi_gpu jacobi.cpp)

target_compile_options(jacobi_gpu PRIVATE
    -acc=gpu
    -Minfo=all
    -O2
    -ta=tesla
)

target_link_libraries(jacobi_gpu
    Boost::program_options
)