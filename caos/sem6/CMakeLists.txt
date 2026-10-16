cmake_minimum_required(VERSION 3.16)
project(linux_memory_seminars_c LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(observe STATIC src/observe.c)
target_include_directories(observe PRIVATE include)

foreach(name IN ITEMS 01_locality 02_mmap)
    add_executable(${name} src/${name}.c)
    target_link_libraries(${name} PRIVATE observe)
    target_include_directories(${name} PRIVATE include)
endforeach()
