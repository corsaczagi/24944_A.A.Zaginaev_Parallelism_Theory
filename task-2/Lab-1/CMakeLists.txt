cmake_minimum_required(VERSION 3.10)
project(MatrixVectorMult)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Устанавливаем тип сборки по умолчанию
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING 
        "Build type (Release, Debug)" FORCE)
endif()

# Поиск OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
    message(STATUS "OpenMP flags: ${OpenMP_CXX_FLAGS}")
    set(USE_OPENMP TRUE)
else()
    message(WARNING "OpenMP not found! The program will compile without parallel support.")
    set(USE_OPENMP FALSE)
endif()

# Создаем исполняемый файл
add_executable(matrix_vector_mult main-1.cpp)

# Линкуем библиотеки
if(USE_OPENMP)
    target_link_libraries(matrix_vector_mult OpenMP::OpenMP_CXX)
else()
    target_link_libraries(matrix_vector_mult pthread)
endif()

# Вывод информации о конфигурации
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
if(USE_OPENMP)
    message(STATUS "OpenMP enabled")
else()
    message(STATUS "OpenMP disabled")
endif()