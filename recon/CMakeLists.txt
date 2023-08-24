# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut PSI
# Author: Jun Zhu
#
# Distributed under the terms of the GPLv3 License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
project(recastx-recon LANGUAGES CXX)

find_package(cppzmq 4.7.1 REQUIRED)
message(STATUS "Found cppzmq ${cppzmq_VERSION} at ${cppzmq_INCLUDE_DIR}")

# Caveat: sequence
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFTW REQUIRED COMPONENTS FLOAT_LIB)

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3 ${Eigen3_VERSION}")

find_package(nlohmann_json REQUIRED)

find_package(TBB 2021.9 REQUIRED)
message(STATUS "Found TBB ${TBB_VERSION}")

include(CheckLanguage)
check_language(CUDA)

add_library(astra-toolbox SHARED IMPORTED)
if (APPLE)
    set_target_properties(
            astra-toolbox PROPERTIES
            IMPORTED_LOCATION ${CMAKE_PREFIX_PATH}/lib/libastra.dylib)
else()
    set_target_properties(
            astra-toolbox PROPERTIES
            IMPORTED_LOCATION ${CMAKE_PREFIX_PATH}/lib/libastra.so
            INTERFACE_COMPILE_DEFINITIONS "ASTRA_CUDA")
endif()

set(ASTRA_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)

set(RECON_SOURCES
        "src/utils.cpp"
        "src/ramp_filter.cpp"
        "src/phase.cpp"
        "src/reconstructor.cpp"
        "src/slice_mediator.cpp"
        "src/daq_client.cpp"
        "src/rpc_server.cpp"
        "src/monitor.cpp"
        "src/projection.cpp"
        "src/application.cpp"
)

set(RECON_LIB _recastx-recon)
add_library(${RECON_LIB} ${RECON_SOURCES})

target_include_directories(${RECON_LIB}
        PRIVATE
                ${ASTRA_INCLUDE_DIR}
        PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_CURRENT_LIST_DIR}/../common/include
        )

target_link_libraries(${RECON_LIB}
        PRIVATE
                nlohmann_json::nlohmann_json
                spdlog::spdlog
                TBB::tbb
                ${FFTW_FLOAT_LIB}
        PUBLIC 
                cppzmq
                spdlog::spdlog
                fmt::fmt
                Eigen3::Eigen
                astra-toolbox
                recastx_grpc_proto
        )

target_compile_options(${RECON_LIB}
        PUBLIC -Wall -Wextra -Wfatal-errors -fPIC -static)

target_compile_definitions(${RECON_LIB} PUBLIC VERBOSITY=${VERBOSITY})

if (BUILD_TEST)
    add_subdirectory(tests)
endif()

set(EXEC_NAME "recastx-recon")
add_executable(${EXEC_NAME} "src/main.cpp")
target_link_libraries(${EXEC_NAME} PUBLIC ${RECON_LIB} Boost::program_options)

install(TARGETS ${EXEC_NAME} RUNTIME DESTINATION bin)