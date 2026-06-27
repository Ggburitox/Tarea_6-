cmake_minimum_required(VERSION 3.20)
project(riscv_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(riscv_sim
    src/main.cpp
    src/cpu.cpp
    src/memory.cpp
    src/decoder.cpp
)

target_include_directories(riscv_sim PRIVATE src)
