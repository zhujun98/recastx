project(consumer LANGUAGES CXX)

set(SOURCE_NAMES
    "src/main.cpp"
    "src/server/server.cpp"
)

set(TARGET_NAME "consumer")
add_executable(${TARGET_NAME} ${SOURCE_NAMES})
target_link_libraries(${TARGET_NAME}
    PRIVATE
        pthread
        stdc++fs
        tomop
)
target_include_directories(${TARGET_NAME} PRIVATE "include")
target_compile_options(${TARGET_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wfatal-errors)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(${TARGET_NAME} PRIVATE -g)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -O3)
endif()
