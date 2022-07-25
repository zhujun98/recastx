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
    "src/utils.cpp"
    "src/filter.cpp"
    "src/phase.cpp"
    "src/solver.cpp"
    "src/reconstructor.cpp"
    "src/data_receiver.cpp"
    "src/broker.cpp"
)

add_library(slicerecon ${SOURCES})

target_include_directories(slicerecon
    PRIVATE
        ${ASTRA_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(slicerecon
    PUBLIC
        tomopackets
        spdlog::spdlog
        Eigen3::Eigen
        astra-toolbox
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        ${TBB_LIBRARIES}
        ${FFTW_FLOAT_LIB}
)

target_compile_options(slicerecon
    PUBLIC
        -Wall -Wextra -Wfatal-errors -fPIC -static)

target_compile_definitions(slicerecon PUBLIC VERBOSITY=${VERBOSITY})

if (BUILD_TEST)
    add_subdirectory(tests)
endif()

set(EXEC_NAME "tomcat-live-server")
add_executable(${EXEC_NAME} "src/server.cpp")
target_link_libraries(${EXEC_NAME} PUBLIC slicerecon Boost::program_options)

install(TARGETS ${EXEC_NAME} RUNTIME DESTINATION bin)