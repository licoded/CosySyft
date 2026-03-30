cmake_minimum_required(VERSION 3.5)
project(Syftmax)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_MODULE_PATH
    "${CMAKE_MODULE_PATH}"
    "${CMAKE_ROOT_DIR}/cmake/Modules"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(cudd REQUIRED)

# CLI11 (header-only argument parsing library)
# Install with: brew install cli11  (macOS) or  apt install libcli11-dev  (Ubuntu)
find_package(CLI11 REQUIRED CONFIG)

set(EXT_LIBRARIES_PATH ${CUDD_LIBRARIES} CLI11::CLI11)
set(EXT_INCLUDE_PATH ${CUDD_INCLUDE_DIRS})

message(STATUS "EXT_LIBRARIES_PATH: ${EXT_LIBRARIES_PATH}")
add_subdirectory(src)
