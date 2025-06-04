cmake_minimum_required(VERSION 3.15)
project(quake C)

# enable vcpkg toolchain if available
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain")
endif()

set(CMAKE_C_STANDARD 17)

add_compile_definitions(GLQUAKE=1)

add_subdirectory(WinQuake)
