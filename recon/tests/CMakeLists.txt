# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut PSI
# Author: Jun Zhu
#
# Distributed under the terms of the GPLv3 License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
configure_file(downloadGTest.cmake.in googletest-download/CMakeLists.txt)

execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -Wno-dev .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
)
if (result)
    message(FATAL_ERROR "CMAKE step for gtest failed: ${result}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
)
if (result)
    message(FATAL_ERROR "BUILD step for gtest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build EXCLUDE_FROM_ALL
)

set(GTEST_INCLUDE_DIRS "${gtest_SOURCE_DIR}/include")
set(GMOCK_INCLUDE_DIRS "${gmock_SOURCE_DIR}/include")

find_package(Threads REQUIRED)

set(UNITTESTS test_buffer.cpp 
              test_tensor.cpp
              test_filter.cpp
              test_preprocessing.cpp
              test_slicemediator.cpp
              test_application.cpp
              )

foreach(filename IN LISTS UNITTESTS)
    get_filename_component(filename ${filename} NAME)
    string(REPLACE ".cpp" "" targetname ${filename})
    add_executable(${targetname} main.cpp ${filename})
    target_include_directories(${targetname}
        PRIVATE
            ${GTEST_INCLUDE_DIRS}
            ${GMOCK_INCLUDE_DIRS}
            )
    target_link_libraries(${targetname}
        PRIVATE
            ${RECON_LIB}
            gtest
            pthread
            )
endforeach()

add_executable(test_recon main.cpp ${UNITTESTS})
target_include_directories(test_recon
    PRIVATE
        ${GTEST_INCLUDE_DIRS} 
        ${GMOCK_INCLUDE_DIRS}
        )
target_link_libraries(test_recon
    PRIVATE
        ${RECON_LIB}
        gtest
        pthread
    )