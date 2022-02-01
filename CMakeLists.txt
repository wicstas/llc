cmake_minimum_required(VERSION 3.12)

project(llc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

include_directories(
    ./src
)

add_library(llc
src/llc/types.cpp
src/llc/misc.cpp
src/llc/tokenizer.cpp
src/llc/parser.cpp
)

add_executable(llc_test 
test/main.cpp
)
target_link_libraries(llc_test PRIVATE llc)