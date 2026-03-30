cmake_minimum_required(VERSION 3.5)
project(Syftmax)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# CUDD — vendored under third_party/cudd/
set(CUDD_DIR "${PROJECT_SOURCE_DIR}/third_party/cudd")
set(CUDD_INCLUDE_DIRS "${CUDD_DIR}/include")
set(CUDD_LIBRARIES    "${CUDD_DIR}/lib/libcudd.a")

# CLI11 — vendored under third_party/cli11/
set(CLI11_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/cli11/include")
add_library(cli11 INTERFACE)
target_include_directories(cli11 INTERFACE "${CLI11_INCLUDE_DIR}")

set(EXT_LIBRARIES_PATH ${CUDD_LIBRARIES} cli11)
set(EXT_INCLUDE_PATH   ${CUDD_INCLUDE_DIRS})

message(STATUS "CUDD_INCLUDE_DIRS: ${CUDD_INCLUDE_DIRS}")
message(STATUS "CUDD_LIBRARIES:    ${CUDD_LIBRARIES}")
add_subdirectory(src)
