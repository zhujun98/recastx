project(tomcat LANGUAGES CXX)

find_package(cppzmq 4.7.1 REQUIRED)
message(STATUS "Found cppzmq ${cppzmq_VERSION} at ${cppzmq_INCLUDE_DIR}")

add_library(tomcat INTERFACE)
target_include_directories(tomcat INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(tomcat INTERFACE cppzmq)
