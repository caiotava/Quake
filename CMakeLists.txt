cmake_minimum_required(VERSION 3.31)
project(quake)

set(CMAKE_CXX_STANDARD 26)

add_compile_definitions(GLQUAKE=1)

add_subdirectory(WinQuake)