cmake_minimum_required(VERSION 3.15.5)

project(tomcat-live LANGUAGES CXX)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)

find_package(nlohmann_json REQUIRED)

add_library(astra-toolbox SHARED IMPORTED)
set_target_properties(
    astra-toolbox PROPERTIES
    IMPORTED_LOCATION ${CMAKE_PREFIX_PATH}/lib/libastra.so
    INTERFACE_COMPILE_DEFINITIONS "ASTRA_CUDA")
set(ASTRA_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)

if (BUILD_PYTHON)
    set(BINDING_NAME "tomcatserver")
    pybind11_add_module(${BINDING_NAME} "python/tomcatserver.cpp" "src/processing.cpp")
    target_include_directories(${BINDING_NAME} PRIVATE "include")
    target_link_libraries(${BINDING_NAME} 
        PRIVATE 
            Eigen3::Eigen pthread astra-toolbox bulk_thread tomopackets nlohmann_json::nlohmann_json)
    target_compile_options(${BINDING_NAME} PRIVATE -Wall -Wfatal-errors -Wextra -std=c++17)

    install(TARGETS ${BINDING_NAME}
            LIBRARY DESTINATION ${Python3_SITELIB})
endif()