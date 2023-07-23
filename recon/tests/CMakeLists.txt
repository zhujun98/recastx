# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut PSI
# Author: Jun Zhu
#
# Distributed under the terms of the GPLv3 License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
set(RECON_TEST_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../common/include
                            ${CMAKE_CURRENT_LIST_DIR}/../include
)
set(RECON_TEST_COMMON_LIBRARIES spdlog::spdlog gmock gtest)

set(SIMPLE_TESTS test_tensor.cpp 
                 test_buffer.cpp
                 test_preprocessing.cpp
)
foreach(filename IN LISTS SIMPLE_TESTS)
    get_filename_component(filename ${filename} NAME)
    string(REPLACE ".cpp" "" targetname ${filename})

    add_executable(${targetname} main.cpp ${filename})
    target_include_directories(${targetname} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
    target_link_libraries(${targetname} PRIVATE ${RECON_TEST_COMMON_LIBRARIES})

    gtest_discover_tests(${targetname})
endforeach()

# filter test
set(TEST_NAME test_filter)
add_executable(${TEST_NAME} main.cpp test_filter.cpp ${CMAKE_CURRENT_LIST_DIR}/../src/ramp_filter.cpp)
target_include_directories(${TEST_NAME} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
target_link_libraries(${TEST_NAME} PRIVATE ${FFTW_FLOAT_LIB} TBB::tbb ${RECON_TEST_COMMON_LIBRARIES})
gtest_discover_tests(${TEST_NAME})

# mediator test
set(TEST_NAME test_slicemediator)
add_executable(${TEST_NAME} main.cpp test_slicemediator.cpp ${CMAKE_CURRENT_LIST_DIR}/../src/slice_mediator.cpp)
target_include_directories(${TEST_NAME} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
target_link_libraries(${TEST_NAME} PRIVATE ${RECON_TEST_COMMON_LIBRARIES})
gtest_discover_tests(${TEST_NAME})

# app test
set(TEST_NAME test_application)
add_executable(${TEST_NAME} main.cpp test_application.cpp 
        ${CMAKE_CURRENT_LIST_DIR}/../src/application.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/slice_mediator.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/phase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/rpc_server.cpp
)
target_include_directories(${TEST_NAME} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
target_link_libraries(${TEST_NAME} PRIVATE cppzmq TBB::tbb Eigen3::Eigen ${FFTW_FLOAT_LIB} recastx_grpc_proto ${RECON_TEST_COMMON_LIBRARIES})
gtest_discover_tests(${TEST_NAME})