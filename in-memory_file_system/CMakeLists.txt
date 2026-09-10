cmake_minimum_required(VERSION 3.10)
project(InMemoryFileSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(fs_demo
    src/main.cpp
    src/FileSystem.cpp
)

target_include_directories(fs_demo PRIVATE include)
