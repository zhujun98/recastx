project(tomopackets LANGUAGES CXX)

find_package(cppzmq 4.7.1 REQUIRED)
message(STATUS "Found cppzmq ${cppzmq_VERSION} at ${cppzmq_INCLUDE_DIR}")

add_library(tomopackets INTERFACE)
target_include_directories(tomopackets INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(tomopackets INTERFACE cppzmq)
