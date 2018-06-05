cmake_minimum_required(VERSION 3.8)
project(project7)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp -lm")

set(SOURCE_FILES
        project7_omp.cpp project7_cl.cpp project7_simd.cpp simd.p5.cpp)

add_executable(project7_omp.cpp ${SOURCE_FILES})