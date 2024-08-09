# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut
# Author: Jun Zhu
#
# Distributed under the terms of the BSD 3-Clause License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
set(RECASTX_RECON_TEST_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../common/include
                                    ${CMAKE_CURRENT_LIST_DIR}/../include
)
set(RECASTX_RECON_TEST_SRC_FILE_DIR ${CMAKE_CURRENT_LIST_DIR}/../src/)

set(RECONX_RECON_TEST_COMMON_LIBRARIES spdlog::spdlog gmock gtest)

set(RECASTX_RECON_TEST_FILES test_tensor.cpp 
                             test_buffer.cpp
                             test_queue.cpp
                             test_preprocessing.cpp
                             test_projection_mediator.cpp
                             test_slice_mediator.cpp
                             test_ramp_filter.cpp
                             test_monitor.cpp
)
set(RECASTX_RECON_TEST_NEED_TBB test_ramp_filter.cpp)
set(RECASTX_RECON_TEST_NEED_FFTW test_ramp_filter.cpp)
set(RECASTX_RECON_TEST_NEED_ZMQ test_monitor.cpp)
foreach(test_file IN LISTS RECASTX_RECON_TEST_FILES)
    get_filename_component(test_filename ${test_file} NAME)
    string(REPLACE ".cpp" "" targetname ${test_filename})
    
    string(REPLACE "test_" "" source_filename ${test_filename})
    set(source_file ${RECASTX_RECON_TEST_SRC_FILE_DIR}/${source_filename})

    if (EXISTS ${source_file})
        set(src_files ${test_filename} ${source_file})
    else()
        set(src_files ${test_filename})
    endif()

    add_executable(${targetname} main.cpp ${src_files})

    target_include_directories(${targetname} PRIVATE ${RECASTX_RECON_TEST_INCLUDE_DIRS})
    target_link_libraries(${targetname} PRIVATE ${RECONX_RECON_TEST_COMMON_LIBRARIES})

    if (${test_file} IN_LIST RECASTX_RECON_TEST_NEED_TBB)
        target_link_libraries(${targetname} PRIVATE TBB::tbb)
    endif()

    if (${test_file} IN_LIST RECASTX_RECON_TEST_NEED_FFTW)
        target_link_libraries(${targetname} PRIVATE ${FFTW_FLOAT_LIB})
    endif()

    if (${test_file} IN_LIST RECASTX_RECON_TEST_NEED_ZMQ)
        target_link_libraries(${targetname} PRIVATE cppzmq)
    endif()

    gtest_discover_tests(${targetname})
endforeach()

# daq client test
set(RECASTX_RECON_DAQ_TEST test_daq_client)
add_executable(${RECASTX_RECON_DAQ_TEST} main.cpp test_daq_client.cpp 
        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/daq/zmq_daq_client.cpp
        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/daq/std_daq_client.cpp
)
target_include_directories(${RECASTX_RECON_DAQ_TEST} PRIVATE ${RECASTX_RECON_TEST_INCLUDE_DIRS})
target_link_libraries(${RECASTX_RECON_DAQ_TEST} 
    PRIVATE
        cppzmq
        nlohmann_json::nlohmann_json
        ${RECONX_RECON_TEST_COMMON_LIBRARIES}
)
gtest_discover_tests(${RECASTX_RECON_DAQ_TEST})

# app test
#set(RECASTX_RECON_APP_TEST test_application)
#add_executable(${RECASTX_RECON_APP_TEST} main.cpp test_application.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/application.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/monitor.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/preprocessor.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/projection_mediator.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/slice_mediator.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/phase.cpp
#        ${RECASTX_RECON_TEST_SRC_FILE_DIR}/rpc_server.cpp
#)
#target_include_directories(${RECASTX_RECON_APP_TEST} PRIVATE ${RECASTX_RECON_TEST_INCLUDE_DIRS})
#target_link_libraries(${RECASTX_RECON_APP_TEST}
#    PRIVATE
#        TBB::tbb
#        Eigen3::Eigen
#        ${FFTW_FLOAT_LIB}
#        recastx_grpc_proto
#        ${RECONX_RECON_TEST_COMMON_LIBRARIES}
#)
#gtest_discover_tests(${RECASTX_RECON_APP_TEST})