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
                 test_slicemediator.cpp
                 test_application.cpp
)
set(APP_TESTS test_slicemediator.cpp test_application.cpp)
foreach(filename IN LISTS SIMPLE_TESTS)
    get_filename_component(filename ${filename} NAME)
    string(REPLACE ".cpp" "" targetname ${filename})

    add_executable(${targetname} main.cpp ${filename})
    
    target_include_directories(${targetname} PRIVATE ${RECON_TEST_INCLUDE_DIRS})

    target_link_libraries(${targetname} PRIVATE ${RECON_TEST_COMMON_LIBRARIES})
    if(filename IN_LIST APP_TESTS)
        target_link_libraries(${targetname} PRIVATE ${RECON_LIB})
    endif()
    
    gtest_discover_tests(${targetname})
endforeach()

# filter test
add_executable(test_filter main.cpp test_filter.cpp ${CMAKE_CURRENT_LIST_DIR}/../src/ramp_filter.cpp)
target_include_directories(test_filter PRIVATE ${RECON_TEST_INCLUDE_DIRS})
target_link_libraries(test_filter PRIVATE ${FFTW_FLOAT_LIB} TBB::tbb ${RECON_TEST_COMMON_LIBRARIES})
gtest_discover_tests(test_filter)
