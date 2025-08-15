
cmake_minimum_required(VERSION 3.10)

project(test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	add_compile_options("/W4" "/WX" "/permissive-")
else()
	add_compile_options("-Wall" "-Wextra" "-Werror" "-pedantic" "-pedantic-errors")
endif()

include_directories("./include")
add_executable(test "./test/main.cpp")