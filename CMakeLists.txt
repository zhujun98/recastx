# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut
# Author: Jun Zhu
#
# Distributed under the terms of the BSD 3-Clause License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.24)

project(recastx LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_GUI "Build OpenGL GUI only" OFF)
option(BUILD_TEST "Build unit test" OFF)
option(VERBOSITY "Build with verbose performance monitoring and debug messages" 1)

# VERBOSITY values:
# - 0: no verbosity
# - 1: general performance benchmarks
# - 2: performance benchmark of critical functions, 
#      monitoring occupation of the memory buffer
# - 3: communications between the GUI and the reconstruction server
# - 4: received image data information (type, frame id, etc.),
#      handling of mouse events in the GUI
if((${VERBOSITY} LESS 0) OR (${VERBOSITY} GREATER 4}))
    message(FATAL_ERROR "Available VERBOSITY values are 0, 1, 2, 3 or 4. Actual: ${VERBOSITY}")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS "-Wno-unused-result")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_INSTALL_PREFIX ${CMAKE_PREFIX_PATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(Boost 1.82 COMPONENTS program_options REQUIRED)

find_package(spdlog 1.10.0 REQUIRED)

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

find_package(gRPC CONFIG 1.51.1 REQUIRED)
message(STATUS "Found gRPC ${gRPC_VERSION}")

add_subdirectory(protos)

if (BUILD_TEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.13.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    add_subdirectory(tests)
endif()

if (BUILD_GUI)
    add_subdirectory(gui)
else()
    add_subdirectory(recon)
endif()