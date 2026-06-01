cmake_minimum_required(VERSION 3.28)
project(compile-time-reflection CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(example
    example/main.cpp
)

target_include_directories(example PRIVATE
    include
    example
)

target_compile_options(example PRIVATE
    -freflection
    -std=c++26
)
