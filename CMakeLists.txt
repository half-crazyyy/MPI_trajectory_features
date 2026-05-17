cmake_minimum_required(VERSION 3.20)

project(
    mpi_trajectory_features
    VERSION 1.1.0
    DESCRIPTION "MPI-based trajectory feature calculation for validation module"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MPI REQUIRED COMPONENTS CXX)

add_executable(mpi_trajectory_features
    src/main.cpp
    src/csv_reader.cpp
    src/trajectory_stats.cpp
    src/distributed_processor.cpp
    src/benchmark.cpp
    include/csv_reader.hpp
    include/trajectory_stats.hpp
    include/distributed_processor.hpp
    include/benchmark.hpp
)

target_include_directories(mpi_trajectory_features
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(mpi_trajectory_features
    PRIVATE
    MPI::MPI_CXX
)

if(MSVC)
    target_compile_options(mpi_trajectory_features PRIVATE /W4 /permissive-)
else()
    target_compile_options(mpi_trajectory_features PRIVATE -Wall -Wextra -Wpedantic)
endif()
