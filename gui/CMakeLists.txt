project(tomcat-live-gui LANGUAGES C CXX)

include(FindOpenGL)
message(STATUS "Found OpenGL at ${OPENGL_LIBRARIES}, ${OPENGL_INCLUDE_DIR}")

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3 ${Eigen3_VERSION}")

# glm
add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/glm glm)

# glfw
add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/glfw glfw)

# gl3w
set(GL3W_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/gl3w)
file(MAKE_DIRECTORY ${GL3W_BUILD_DIR})
message(STATUS ${GL3W_BUILD_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} -G
                ${CMAKE_GENERATOR} ${PROJECT_SOURCE_DIR}/../ext/gl3w
                WORKING_DIRECTORY ${GL3W_BUILD_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} --build .
                WORKING_DIRECTORY ${GL3W_BUILD_DIR})

# imgui
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/../ext/imgui")
set(IMGUI_SOURCE_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${GL3W_BUILD_DIR}/src/gl3w.c
)
set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-w")
add_library(imgui ${IMGUI_SOURCE_FILES})
target_include_directories(imgui
    PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${GL3W_BUILD_DIR}/include
)

target_link_libraries(imgui PUBLIC dl glfw OpenGL::GL)

set(SOURCE_NAMES
    "src/main.cpp"
    "src/math_common.cpp"
    "src/input.cpp"
    "src/util.cpp"
    "src/window.cpp"
    "src/path.cpp"
    "src/server.cpp"
    "src/packet_publisher.cpp"
    "src/utils/recorder.cpp"
    "src/graphics/primitives.cpp"
    "src/graphics/shader_program.cpp"
    "src/graphics/slice.cpp"
    "src/graphics/volume.cpp"
    "src/graphics/renderer.cpp"
    "src/graphics/render_target.cpp"
    "src/graphics/interface.cpp"
    "src/graphics/recon_component.cpp"
    "src/graphics/geom_component.cpp"
    "src/graphics/axes_component.cpp"
    "src/graphics/partitioning_component.cpp"
    "src/scenes/scene.cpp"
    "src/scenes/scene2d.cpp"
    "src/scenes/overview_scene.cpp"
    "src/scenes/scene_camera.cpp"
    "src/scenes/scene_camera2d.cpp"
    "src/scenes/scene_camera3d.cpp"
)

set(TARGET_NAME tomcat-live-gui)
add_executable(${TARGET_NAME} ${SOURCE_NAMES})
target_link_libraries(${TARGET_NAME}
    PRIVATE
        imgui
        pthread
        glm
        Eigen3::Eigen
        Boost::program_options
        spdlog::spdlog
        tomcat
)
if (NOT APPLE)
    target_link_libraries(${TARGET_NAME} PRIVATE X11)
endif()
target_include_directories(${TARGET_NAME} PRIVATE "include")
target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wfatal-errors)
# This is to work around GLM issue#754 <https://github.com/g-truc/glm/issues/754>
target_compile_definitions(${TARGET_NAME} 
    PUBLIC
        -DVERBOSITY=${VERBOSITY}
    PRIVATE
        -DGLM_ENABLE_EXPERIMENTAL
        -DGLM_FORCE_CTOR_INIT
)

# INSTALL COMMANDS
install(TARGETS ${TARGET_NAME} imgui
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
