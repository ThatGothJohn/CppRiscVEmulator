cmake_minimum_required(VERSION 3.13)
project(cppRV64)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Werror")

set(SOURCE_FILES src/main.cpp src/cpu.cpp src/cpu.h src/memory.cpp src/memory.h src/bus.cpp src/bus.h)

add_executable(cppRV64 ${SOURCE_FILES})