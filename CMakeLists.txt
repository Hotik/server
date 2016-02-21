
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
SET(LINKER_FLAGS "${LINKER_FLAGS} -lev")
add_executable(main main.cpp )
