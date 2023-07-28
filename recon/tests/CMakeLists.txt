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

set(TEST_FILES test_tensor.cpp 
               test_buffer.cpp
               test_preprocessing.cpp
               test_slice_mediator.cpp
               test_ramp_filter.cpp
)
set(TESTS_WITH_TBB test_ramp_filter.cpp)
set(TESTS_WITH_FFTW test_ramp_filter.cpp)
foreach(test_file IN LISTS TEST_FILES)
    get_filename_component(test_filename ${test_file} NAME)
    string(REPLACE ".cpp" "" targetname ${test_filename})
    
    string(REPLACE "test_" "" source_filename ${test_filename})
    set(source_file ${CMAKE_CURRENT_LIST_DIR}/../src/${source_filename})

    if (EXISTS ${source_file})
        add_executable(${targetname} main.cpp ${test_filename} ${source_file})
    else()
        add_executable(${targetname} main.cpp ${test_filename})
    endif()
    
    target_include_directories(${targetname} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
    target_link_libraries(${targetname} PRIVATE ${RECON_TEST_COMMON_LIBRARIES})

    if (${test_file} IN_LIST TESTS_WITH_TBB)
        target_link_libraries(${targetname} PRIVATE TBB::tbb)
    endif()

    if (${test_file} IN_LIST TESTS_WITH_FFTW)
        target_link_libraries(${targetname} PRIVATE ${FFTW_FLOAT_LIB})
    endif()

    gtest_discover_tests(${targetname})
endforeach()

# app test
set(TEST_NAME test_application)
add_executable(${TEST_NAME} main.cpp test_application.cpp 
        ${CMAKE_CURRENT_LIST_DIR}/../src/application.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/monitor.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/projection.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/slice_mediator.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/phase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../src/rpc_server.cpp
)
target_include_directories(${TEST_NAME} PRIVATE ${RECON_TEST_INCLUDE_DIRS})
target_link_libraries(${TEST_NAME} PRIVATE cppzmq TBB::tbb Eigen3::Eigen ${FFTW_FLOAT_LIB} recastx_grpc_proto ${RECON_TEST_COMMON_LIBRARIES})
gtest_discover_tests(${TEST_NAME})