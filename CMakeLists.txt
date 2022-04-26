cmake_minimum_required(VERSION 3.15.5)

project(recast LANGUAGES C CXX)

option(BUILD_GUI "Build OpenGL GUI recast3d" OFF)
option(BUILD_TEST "Build unit test" OFF)
option(BUILD_PYTHON "Build Python bindings" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_INSTALL_PREFIX ${CMAKE_PREFIX_PATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if (BUILD_PYTHON)
    find_package(pybind11 REQUIRED)
    find_package(Python3 REQUIRED)
endif()

add_subdirectory(tomopackets)
add_subdirectory(slicerecon)
add_subdirectory(consumer)

if (BUILD_GUI)
    add_subdirectory(recast3d)
endif()