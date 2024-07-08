# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut
# Author: Jun Zhu
#
# Distributed under the terms of the BSD 3-Clause License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
project(recastx-recon LANGUAGES CXX CUDA)

if (CMAKE_CUDA_COMPILER)
    message(STATUS "CUDA compiler: ${CMAKE_CUDA_COMPILER}")

    find_package(CUDAToolkit REQUIRED)
    enable_language(CUDA)
else()
    message(FATAL_ERROR "NO CUDA support")
endif()

find_package(cppzmq 4.7.1 REQUIRED)
message(STATUS "Found cppzmq ${cppzmq_VERSION} at ${cppzmq_INCLUDE_DIR}")

# Caveat: sequence
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFTW REQUIRED COMPONENTS FLOAT_LIB)

find_package(nlohmann_json REQUIRED)

find_package(TBB 2021.9 REQUIRED)
message(STATUS "Found TBB ${TBB_VERSION}")

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

set(RECON_CUDA_SOURCES "src/cuda/recon3d.cu"
                       "src/cuda/memory.cu"
                       "src/cuda/sinogram.cu")
set(RECON_CUDA_LIB _recastx-recon-cuda)
add_library(${RECON_CUDA_LIB} ${RECON_CUDA_SOURCES})
target_link_libraries(${RECON_CUDA_LIB} PRIVATE spdlog::spdlog astra-toolbox)
target_include_directories(${RECON_CUDA_LIB} PRIVATE
        ${ASTRA_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/../common/include
)
set_target_properties(${RECON_CUDA_LIB} PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

set(RECON_SOURCES
        "src/utils.cpp"
        "src/ramp_filter.cpp"
        "src/phase.cpp"
        "src/preprocessor.cpp"
        "src/reconstructor.cpp"
        "src/projection_mediator.cpp"
        "src/slice_mediator.cpp"
        "src/rpc_server.cpp"
        "src/monitor.cpp"
        "src/application.cpp"
        "src/daq/std_daq_client.cpp"
        "src/daq/zmq_daq_client.cpp"
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
                ${RECON_CUDA_LIB}
                nlohmann_json::nlohmann_json
                spdlog::spdlog
                TBB::tbb
                ${FFTW_FLOAT_LIB}
        PUBLIC 
                cppzmq
                Eigen3::Eigen
                astra-toolbox
                recastx_grpc_proto
        )

if (BENCHMARK)
    target_link_libraries(${RECON_LIB} PRIVATE nvtx3-cpp)
    target_compile_definitions(${RECON_LIB} PRIVATE BENCHMARK)
endif()

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