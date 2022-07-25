cmake_minimum_required(VERSION 3.15.5)

project(tomcat-live LANGUAGES C CXX)

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
       monitoring occupation of the memory buffer
# - 3: received image data information (type, frame id, etc.)
if((${VERBOSITY} LESS 0) OR (${VERBOSITY} GREATER 3}))
    message(FATAL_ERROR "Available VERBOSITY values are 0, 1, 2 or 3. Actual: ${VERBOSITY}")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS "-Wno-unused-result")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_INSTALL_PREFIX ${CMAKE_PREFIX_PATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(Boost 1.61 COMPONENTS program_options REQUIRED)

find_package(spdlog REQUIRED)

add_subdirectory(tomopackets)

if (BUILD_GUI)
    add_subdirectory(gui)
else()
    add_subdirectory(slicerecon)
    add_subdirectory(consumer)
endif()
