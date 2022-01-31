project(slicerecon LANGUAGES CXX)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3 ${Eigen3_VERSION}")

add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/bulk bulk)

add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/flags flags)

add_library(astra-toolbox SHARED IMPORTED)
set_target_properties(
    astra-toolbox PROPERTIES
    IMPORTED_LOCATION ${CMAKE_PREFIX_PATH}/lib/libastra.so
    INTERFACE_COMPILE_DEFINITIONS "ASTRA_CUDA")
set(ASTRA_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFTW REQUIRED COMPONENTS FLOAT_LIB)

add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/pybind11 pybind11)

set(SOURCES
    "src/util/util.cpp"
    "src/util/log.cpp"
    "src/util/bench.cpp"
    "src/util/processing.cpp"
    "src/reconstruction/reconstructor.cpp"
    "src/reconstruction/helpers.cpp"
)
set(TARGET_NAME "slicerecon")

add_library(${TARGET_NAME} ${SOURCES})
target_include_directories(${TARGET_NAME}
    PUBLIC
        ${ASTRA_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${TARGET_NAME}
    PUBLIC
        "astra-toolbox"
        "tomop"
        "Eigen3::Eigen"
        "flags"
        "bulk_thread"
        ${FFTW_FLOAT_LIB}
)
target_compile_options(${TARGET_NAME}
    PRIVATE
        -std=c++17 -Wall -Wextra -Wfatal-errors -fPIC -static)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(${TARGET_NAME} PRIVATE -g)
else()
    set(${TARGET_NAME} PRIVATE -O3)
endif()

add_executable(slicerecon_server "src/slicerecon_server.cpp")
target_link_libraries(slicerecon_server PRIVATE slicerecon)

set(BINDING_NAME "py_slicerecon")
set(BINDING_SOURCES "slicerecon/module.cpp")
pybind11_add_module(${BINDING_NAME} ${BINDING_SOURCES})
target_link_libraries(${BINDING_NAME} PRIVATE slicerecon)

# INSTALL COMMANDS
install(TARGETS slicerecon_server
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
