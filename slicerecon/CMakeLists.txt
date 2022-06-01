project(tomcat-live-slicerecon LANGUAGES CXX)

# Caveat: sequence
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFTW REQUIRED COMPONENTS FLOAT_LIB)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3 ${Eigen3_VERSION}")

find_package(nlohmann_json REQUIRED)

find_package(TBB 2021.5 REQUIRED)

add_library(astra-toolbox SHARED IMPORTED)
set_target_properties(
    astra-toolbox PROPERTIES
    IMPORTED_LOCATION ${CMAKE_PREFIX_PATH}/lib/libastra.so
    INTERFACE_COMPILE_DEFINITIONS "ASTRA_CUDA")
set(ASTRA_INCLUDE_DIR ${CMAKE_PREFIX_PATH}/include)

set(SOURCES
    "src/tomcat_live_server.cpp"
    "src/utils.cpp"
    "src/projection_processor.cpp"
    "src/listener.cpp"
    "src/solver.cpp"
    "src/reconstructor.cpp"
    "src/helpers.cpp"
)

set(TARGET_NAME "tomcat-live-server")
add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${ASTRA_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        astra-toolbox
        tomopackets
        Eigen3::Eigen
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Boost::program_options
        ${TBB_LIBRARIES}
        ${FFTW_FLOAT_LIB}
)

target_compile_options(${TARGET_NAME}
    PRIVATE
        -Wall -Wextra -Wfatal-errors -fPIC -static)

if (BUILD_MONITOR)
    target_compile_definitions(${TARGET_NAME} PRIVATE WITH_MONITOR)
endif()

if (BUILD_TEST)
    add_subdirectory(tests)
endif()

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)