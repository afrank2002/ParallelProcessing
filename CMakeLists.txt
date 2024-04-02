cmake_minimum_required(VERSION 3.27)
project(ParallelProcessing)

# Set the C++ standard for the project
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
# Find the MPI package
find_package(MPI REQUIRED)

# Define the executable target 'ParallelProcessing' and specify the source files
add_executable(ParallelProcessing
        Project3/openMPI.cpp
        Project3/Utils.h
        Project3/Utils.cpp
)

# Link MPI libraries to the 'ParallelProcessing' target
# Use the MPI::MPI_CXX target, which automatically sets include directories and link libraries
target_link_libraries(ParallelProcessing MPI::MPI_CXX)
