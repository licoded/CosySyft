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
# Tried in order: system install, vendored copy in Lydia submodule.
find_package(CLI11 QUIET CONFIG)
if(CLI11_FOUND)
    message(STATUS "Found system CLI11")
    set(CLI11_TARGET CLI11::CLI11)
else()
    set(CLI11_VENDOR_DIR "${PROJECT_SOURCE_DIR}/submodules/lydia/third_party/CLI11/include")
    if(EXISTS "${CLI11_VENDOR_DIR}/CLI/CLI.hpp")
        message(STATUS "Using vendored CLI11 from submodules/lydia/third_party/CLI11")
        add_library(cli11_vendor INTERFACE)
        target_include_directories(cli11_vendor INTERFACE "${CLI11_VENDOR_DIR}")
        set(CLI11_TARGET cli11_vendor)
    else()
        message(FATAL_ERROR
            "CLI11 not found. Either:\n"
            "  1. Install it:  brew install cli11\n"
            "  2. Or initialize the Lydia submodule (only CLI11 headers are needed):\n"
            "     git submodule update --init submodules/lydia")
    endif()
endif()

set(EXT_LIBRARIES_PATH ${CUDD_LIBRARIES} ${CLI11_TARGET})
set(EXT_INCLUDE_PATH ${CUDD_INCLUDE_DIRS})

message(STATUS "EXT_LIBRARIES_PATH: ${EXT_LIBRARIES_PATH}")
add_subdirectory(src)
