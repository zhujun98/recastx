project(tomopackets LANGUAGES CXX)

find_package(cppzmq 4.7.1 REQUIRED)
message(STATUS "Found cppzmq ${cppzmq_VERSION} at ${cppzmq_INCLUDE_DIR}")

add_library(tomopackets INTERFACE)
target_include_directories(tomopackets INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(tomopackets INTERFACE cppzmq)

if (BUILD_TEST)
    set(SOURCE_NAMES "test/test_server.cpp")
    add_executable(test_server ${SOURCE_NAMES})
    target_include_directories(test_server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
    target_link_libraries(test_server PRIVATE cppzmq pthread Boost::boost)
    target_compile_options(${TARGET_NAME} PRIVATE -std=c++14 -Wall -Wextra -Wfatal-errors)
endif()

if (BUILD_PYTHON)
    set(BINDING_NAME "tomop")
    pybind11_add_module(${BINDING_NAME} "python/tomop.cpp")
    target_include_directories(${BINDING_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
    target_link_libraries(${BINDING_NAME} PRIVATE cppzmq pthread Boost::boost)
    target_compile_options(${BINDING_NAME} PRIVATE -std=c++14 -Wall -Wextra -Wfatal-errors)

    install(TARGETS ${BINDING_NAME}
            LIBRARY DESTINATION ${Python3_SITELIB})
endif()
