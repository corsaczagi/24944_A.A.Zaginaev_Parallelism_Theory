cmake_minimum_required(VERSION 3.15)
project(HeatEquationSolver LANGUAGES CXX)

# Устанавливаем компилятор NVIDIA HPC SDK
set(CMAKE_CXX_COMPILER "pgc++")
set(CMAKE_CXX_FLAGS "-fast -O3 -Minfo=all")

# Находим Boost
find_package(Boost REQUIRED COMPONENTS program_options)

# Создаём исполняемый файл
add_executable(heat_solver main.cpp)
target_link_libraries(heat_solver ${Boost_LIBRARIES})

# Цели для разных режимов OpenACC
add_custom_target(build_cpu
    COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="-acc=host -Minfo=all" ${CMAKE_MAKE_PROGRAM}
    COMMENT "Building for CPU (host)")

add_custom_target(build_multicore
    COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="-acc=multicore -Minfo=all" ${CMAKE_MAKE_PROGRAM}
    COMMENT "Building for multicore CPU")

add_custom_target(build_gpu
    COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="-acc=gpu -Minfo=all" ${CMAKE_MAKE_PROGRAM}
    COMMENT "Building for GPU")