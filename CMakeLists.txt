
cmake_minimum_required(VERSION 3.10)

project(lab3)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(lab3
    src/main.cpp
    src/batcher.cpp
    src/threads.cpp
    src/benchmark.cpp
)

target_link_libraries(lab3 PRIVATE Threads::Threads)
