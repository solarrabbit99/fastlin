cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3")

project(fastlin VERSION 1.0 LANGUAGES CXX)

# main engine
set(SOURCE
  "src/fastlin.cpp"
)

add_executable(fastlin ${SOURCE})

target_include_directories(fastlin PRIVATE "include")