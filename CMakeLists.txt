cmake_minimum_required(VERSION 3.12)

project(llc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

include_directories(
    ./include
)

add_executable(llc_test
test/main.cpp
)