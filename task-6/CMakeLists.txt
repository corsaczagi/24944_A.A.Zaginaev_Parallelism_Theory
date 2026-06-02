cmake_minimum_required(VERSION 3.15)
project(HeatEquationSolver LANGUAGES CXX)

# Устанавливаем компилятор NVIDIA HPC SDK
set(CMAKE_CXX_COMPILER "pgc++")
set(CMAKE_CXX_FLAGS "-O3 -Minfo=all")

# Базовый ключ OpenACC (можно переопределить через командную строку)
if(NOT DEFINED OPENACC_MODE)
    set(OPENACC_MODE "gpu")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -acc=${OPENACC_MODE}")

# Находим Boost
find_package(Boost REQUIRED COMPONENTS program_options)

# Создаём исполняемый файл
add_executable(heat_solver main.cpp)
target_link_libraries(heat_solver ${Boost_LIBRARIES})

# Выводим информацию о сборке
message(STATUS "OpenACC mode: ${OPENACC_MODE}")
message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")