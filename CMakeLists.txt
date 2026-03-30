cmake_minimum_required(VERSION 3.5)
project(Syftmax)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# ── Submodule mode ─────────────────────────────────────────────────────────────
# When embedded via add_subdirectory from a parent project, set:
#   COSYSYFT_AS_SUBMODULE=ON
#   COSYSYFT_CUDD_INCLUDE_DIRS  — path(s) to cudd headers (cuddObj.hh etc.)
#   COSYSYFT_CUDD_LIBRARY       — the prebuilt libcudd.a that already includes
#                                  the C++ wrapper (cuddObj.o)
# The parent's CUDD library is used directly; cuddObj.cc is NOT recompiled.
# ──────────────────────────────────────────────────────────────────────────────
option(COSYSYFT_AS_SUBMODULE "Build CosySyft embedded in a parent CMake project" OFF)

if(NOT COSYSYFT_AS_SUBMODULE)
  set(CMAKE_BUILD_TYPE Release)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

  # CUDD — vendored under third_party/cudd/
  set(CUDD_DIR "${PROJECT_SOURCE_DIR}/third_party/cudd")
  set(CUDD_INCLUDE_DIRS "${CUDD_DIR}/include")

  add_library(cudd_cpp STATIC "${CUDD_DIR}/src/cuddObj.cc")
  target_include_directories(cudd_cpp PUBLIC "${CUDD_INCLUDE_DIRS}"
                                      PRIVATE "${CUDD_DIR}/internal")
  target_link_libraries(cudd_cpp PUBLIC "${CUDD_DIR}/lib/libcudd.a")

  set(EXT_LIBRARIES_PATH cudd_cpp)
  set(EXT_INCLUDE_PATH   ${CUDD_INCLUDE_DIRS})
else()
  # Submodule: parent supplies CUDD headers and the full libcudd.a
  if(NOT DEFINED COSYSYFT_CUDD_INCLUDE_DIRS OR NOT DEFINED COSYSYFT_CUDD_LIBRARY)
    message(FATAL_ERROR "COSYSYFT_AS_SUBMODULE=ON requires COSYSYFT_CUDD_INCLUDE_DIRS and COSYSYFT_CUDD_LIBRARY")
  endif()
  set(EXT_INCLUDE_PATH   ${COSYSYFT_CUDD_INCLUDE_DIRS})
  set(EXT_LIBRARIES_PATH ${COSYSYFT_CUDD_LIBRARY})
endif()

# CLI11 — vendored under third_party/cli11/
set(CLI11_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/third_party/cli11/include")
add_library(cli11 INTERFACE)
target_include_directories(cli11 INTERFACE "${CLI11_INCLUDE_DIR}")
list(APPEND EXT_LIBRARIES_PATH cli11)

message(STATUS "CosySyft CUDD include: ${EXT_INCLUDE_PATH}")
add_subdirectory(src)
