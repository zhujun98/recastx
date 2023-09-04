# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut PSI
# Author: Jun Zhu
#
# Distributed under the terms of the GPLv3 License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
set(RECASTX_TEST_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../common/include)

set(RECASTX_TEST_COMMON_LIBRARIES spdlog::spdlog gmock gtest)

set(RECASTX_TEST_FILES test_utils.cpp
)

foreach(test_file IN LISTS RECASTX_TEST_FILES)
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

    target_include_directories(${targetname} PRIVATE ${RECASTX_TEST_INCLUDE_DIRS})
    target_link_libraries(${targetname} PRIVATE ${RECASTX_TEST_COMMON_LIBRARIES})

    gtest_discover_tests(${targetname})
endforeach()
