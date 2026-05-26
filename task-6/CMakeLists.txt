cmake_minimum_required(VERSION 3.16)

project(heat_equation LANGUAGES CXX)

set(CMAKE_CXX_COMPILER nvc++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(heat jacobi.cpp)

target_include_directories(heat PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(heat PRIVATE Boost::program_options)

target_compile_options(heat PRIVATE
    -acc
    -Minfo=all
)

target_link_options(heat PRIVATE
    -acc
)