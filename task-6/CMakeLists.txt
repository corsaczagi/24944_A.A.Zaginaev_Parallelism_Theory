cmake_minimum_required(VERSION 3.20)

project(Task6_OpenACC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(heat heat.cpp)

option(USE_BOOST "Use Boost.Program_options" ON)

if(USE_BOOST)

    find_package(Boost COMPONENTS program_options)

    if(Boost_FOUND)

        target_compile_definitions(heat PRIVATE USE_BOOST)

        target_link_libraries(
            heat PRIVATE
            Boost::program_options
        )

    endif()

endif()

option(OPENACC_GPU "GPU mode" OFF)
option(OPENACC_MULTICORE "Multicore mode" OFF)
option(OPENACC_HOST "Host mode" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "NVHPC")

    target_compile_options(
        heat PRIVATE
        -O2
        -Minfo=all
    )

    if(OPENACC_GPU)

        target_compile_options(
            heat PRIVATE
            -acc=gpu
        )

    elseif(OPENACC_MULTICORE)

        target_compile_options(
            heat PRIVATE
            -acc=multicore
        )

    elseif(OPENACC_HOST)

        target_compile_options(
            heat PRIVATE
            -acc=host
        )

    else()

        target_compile_options(
            heat PRIVATE
            -acc
        )

    endif()

endif()