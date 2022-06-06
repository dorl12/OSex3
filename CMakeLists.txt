cmake_minimum_required(VERSION 3.16)
project(OSex3)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(OSex3 ex3.cpp)
