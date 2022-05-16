project(consumer LANGUAGES CXX)

set(SOURCE_NAMES
    "src/main.cpp"
    "src/server/server.cpp"
)

set(TARGET_NAME "tomcat-live-consumer")
add_executable(${TARGET_NAME} ${SOURCE_NAMES})
target_link_libraries(${TARGET_NAME}
    PRIVATE
        pthread
        stdc++fs
        tomopackets
        Boost::program_options
)
target_include_directories(${TARGET_NAME} PRIVATE "include")
target_compile_options(${TARGET_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wfatal-errors)

# INSTALL COMMANDS
install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)