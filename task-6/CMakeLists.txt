cmake_minimum_required(VERSION 3.16)

project(task6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# PGI / NVIDIA HPC compiler

set(CMAKE_CXX_COMPILER pgc++)

# Общие флаги

set(COMMON_FLAGS "-O3 -Minfo=all")

# =========================

# HOST

# =========================

add_executable(jacobi_host ../jacobi.cpp)

target_compile_options(jacobi_host PRIVATE
-acc=host
${COMMON_FLAGS}
)

target_link_options(jacobi_host PRIVATE
-acc=host
)

# =========================

# MULTICORE

# =========================

add_executable(jacobi_multi ../jacobi.cpp)

target_compile_options(jacobi_multi PRIVATE
-acc=multicore
${COMMON_FLAGS}
)

target_link_options(jacobi_multi PRIVATE
-acc=multicore
)

# =========================

# GPU

# =========================

add_executable(jacobi_gpu ../jacobi.cpp)

target_compile_options(jacobi_gpu PRIVATE
-acc=gpu
${COMMON_FLAGS}
)

target_link_options(jacobi_gpu PRIVATE
-acc=gpu
)
