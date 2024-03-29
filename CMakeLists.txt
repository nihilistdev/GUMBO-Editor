cmake_minimum_required(VERSION 3.16)

project(GUMBO CXX)
project(GUMBO VERSION 1.0.0)

add_subdirectory(src)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_BUILD_TYPE Debug)

LIST(APPEND included_headers 
    "cstdio" "cstdarg" "iostream" "csignal" "cstdlib" "ctime" "cstring" "cerrno" "fcntl.h" "unistd.h" "termios.h" "curses.h" )

foreach(header ${included_headers}) 
   find_path( header PATHS /usr/include)
    if(NOT header)
        message(FATAL_ERROR "Can't find folder containing ${header}")
    else()
        message("-- ${header} -- found")
    endif()
endforeach()

add_compile_options("-g")
add_subdirectory(lib/logger)
