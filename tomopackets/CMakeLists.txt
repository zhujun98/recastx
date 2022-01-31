project(tomopackets LANGUAGES CXX)

option(TOMOP_LIB_ONLY "Do not build python bindings" OFF)
option(BUILD_TEST "Build unit test" OFF)

find_package(cppzmq REQUIRED)
message(STATUS "Found cppzmq ${cppzmq_VERSION} at ${cppzmq_INCLUDE_DIR}")

find_package(Boost 1.61 REQUIRED)

add_library(tomop INTERFACE)
target_include_directories(tomop INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(tomop INTERFACE cppzmq)

add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/pybind11 pybind11)

if (BUILD_TEST)
    set(SOURCE_NAMES "test/test_server.cpp")

    add_executable(test_server ${SOURCE_NAMES})
    target_include_directories(test_server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
    target_link_libraries(test_server PRIVATE cppzmq pthread Boost::boost)
    target_compile_options(${TARGET_NAME} PRIVATE -std=c++14 -Wall -Wextra -Wfatal-errors)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        set(${TARGET_NAME} PRIVATE -g)
    else()
        set(${TARGET_NAME} PRIVATE -O3)
    endif()
endif()

if (NOT TOMOP_LIB_ONLY)
    set(BINDING_NAME "py_tomop")
    set(BINDING_SOURCES "tomop/module.cpp")

    pybind11_add_module(${BINDING_NAME} ${BINDING_SOURCES})
    target_include_directories(${BINDING_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
    target_link_libraries(${BINDING_NAME} PRIVATE cppzmq pthread Boost::boost)
    target_compile_options(${BINDING_NAME} PRIVATE -std=c++14 -Wall -Wextra -Wfatal-errors)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        set(${TARGET_NAME} PRIVATE -g)
    else()
        set(${TARGET_NAME} PRIVATE -O3)
    endif()
endif()
