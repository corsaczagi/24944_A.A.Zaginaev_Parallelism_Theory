cmake_minimum_required(VERSION 3.10)
project(Jacobi2D)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для разных компиляторов
if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI" OR CMAKE_CXX_COMPILER_ID STREQUAL "NVIDIA")
    # NVIDIA HPC SDK (pgc++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -acc -Minfo=all")
    
    # CPU многоядерный
    add_executable(jacobi_cpu jacobi.cpp)
    target_compile_options(jacobi_cpu PRIVATE -ta=multicore)
    
    # GPU (если есть)
    add_executable(jacobi_gpu jacobi.cpp)
    target_compile_options(jacobi_gpu PRIVATE -ta=nvidia)
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC (ограниченная поддержка OpenACC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenacc -O3")
    add_executable(jacobi jacobi.cpp)
    
else()
    # Clang, MSVC и другие (без OpenACC)
    add_executable(jacobi jacobi.cpp)
endif()

# Последовательная версия (без OpenACC)
add_executable(jacobi_seq jacobi.cpp)