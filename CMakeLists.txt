cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++") # or clang for C
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_CXX_STANDARD 14)

project(ParallelProcessing)


# Define the target 'ParallelProcessing' and specify the source files
add_executable(ParallelProcessing
        Project2/openMP.cpp
)

# Find the OpenMP package
find_package(OpenMP)

# If OpenMP is found, apply the flags to the target 'ParallelProcessing'
if(OpenMP_CXX_FOUND)
    target_compile_options(ParallelProcessing PRIVATE ${OpenMP_CXX_FLAGS})
    target_link_libraries(ParallelProcessing PRIVATE ${OpenMP_CXX_LIBRARIES} -stdlib=libc++ -lc++)
endif()

