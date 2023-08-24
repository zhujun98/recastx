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
set(RECON_SRC_FILE_DIR ${CMAKE_CURRENT_LIST_DIR}/../src/)

set(RECON_TEST_COMMON_LIBRARIES spdlog::spdlog fmt::fmt gmock gtest)

set(TEST_FILES test_tensor.cpp 
               test_buffer.cpp
               test_preprocessing.cpp
               test_projection_mediator.cpp
               test_slice_mediator.cpp
               test_ramp_filter.cpp
               test_monitor.cpp
)
set(TESTS_NEED_TBB test_ramp_filter.cpp)
set(TESTS_NEED_FFTW test_ramp_filter.cpp)
set(TESTS_NEED_ZMQ test_monitor.cpp)
foreach(test_file IN LISTS TEST_FILES)
    get_filename_component(test_filename ${test_file} NAME)
    string(REPLACE ".cpp" "" targetname ${test_filename})
    
    string(REPLACE "test_" "" source_filename ${test_filename})
    set(source_file ${RECON_SRC_FILE_DIR}/${source_filename})

    if (EXISTS ${source_file})
        set(src_files ${test_filename} ${source_file})
    else()
        set(src_files ${test_filename})
    endif()

    add_executable(${targetname} main.cpp ${src_files})

    target_include_directories(${targetname} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
    target_link_libraries(${targetname} PRIVATE ${RECON_TEST_COMMON_LIBRARIES})

    if (${test_file} IN_LIST TESTS_NEED_TBB)
        target_link_libraries(${targetname} PRIVATE TBB::tbb)
    endif()

    if (${test_file} IN_LIST TESTS_NEED_FFTW)
        target_link_libraries(${targetname} PRIVATE ${FFTW_FLOAT_LIB})
    endif()

    if (${test_file} IN_LIST TESTS_NEED_ZMQ)
        target_link_libraries(${targetname} PRIVATE cppzmq)
    endif()

    gtest_discover_tests(${targetname})
endforeach()

# app test
set(TEST_NAME test_application)
add_executable(${TEST_NAME} main.cpp test_application.cpp 
        ${RECON_SRC_FILE_DIR}/application.cpp
        ${RECON_SRC_FILE_DIR}/monitor.cpp
        ${RECON_SRC_FILE_DIR}/projection_mediator.cpp
        ${RECON_SRC_FILE_DIR}/slice_mediator.cpp
        ${RECON_SRC_FILE_DIR}/phase.cpp
        ${RECON_SRC_FILE_DIR}/rpc_server.cpp
)
target_include_directories(${TEST_NAME} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
target_link_libraries(${TEST_NAME} PRIVATE cppzmq TBB::tbb Eigen3::Eigen ${FFTW_FLOAT_LIB} recastx_grpc_proto ${RECON_TEST_COMMON_LIBRARIES})
gtest_discover_tests(${TEST_NAME})