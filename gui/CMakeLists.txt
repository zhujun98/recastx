project(tomcat-live-gui LANGUAGES C CXX)

include(FindOpenGL)
message(STATUS "Found OpenGL at ${OPENGL_LIBRARIES}, ${OPENGL_INCLUDE_DIR}")

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3 ${Eigen3_VERSION}")

add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/glm glm)

add_subdirectory(${PROJECT_SOURCE_DIR}/../ext/glfw glfw)
set(glfw3_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/../ext/glfw/include)

set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/../ext/imgui")
set(IMGUI_SOURCE_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp"
    "${IMGUI_DIR}/examples/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/examples/libs/gl3w/GL/gl3w.c"
)
set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-w")
add_library(imgui ${IMGUI_SOURCE_FILES})
target_include_directories(imgui
    PUBLIC
        "${IMGUI_DIR}"
        "${IMGUI_DIR}/examples/libs/gl3w/"
        "${IMGUI_DIR}/examples/opengl3_example/")

target_link_libraries(imgui PUBLIC dl glfw OpenGL::GL)

set(SOURCE_NAMES
    "src/main.cpp"
    "src/math_common.cpp"
    "src/input.cpp"
    "src/util.cpp"
    "src/scene.cpp"
    "src/path.cpp"
    "src/server.cpp"
    "src/graphics/primitives.cpp"
    "src/graphics/scene_object.cpp"
    "src/graphics/scene_object_2d.cpp"
    "src/graphics/scene_object_3d.cpp"
    "src/graphics/scene_camera.cpp"
    "src/graphics/scene_camera_2d.cpp"
    "src/graphics/scene_camera_3d.cpp"
    "src/graphics/shader_program.cpp"
    "src/graphics/slice.cpp"
    "src/graphics/renderer.cpp"
    "src/graphics/render_target.cpp"
    "src/graphics/interface.cpp"
    "src/graphics/components/reconstruction_component.cpp"
    "src/graphics/components/geometry_component.cpp"
    "src/graphics/components/axes_component.cpp"
    "src/graphics/components/control_component.cpp"
    "src/graphics/components/partitioning_component.cpp"
    "src/graphics/components/movie/recorder.cpp"
)

set(TARGET_NAME "tomcat-live-gui")
add_executable(${TARGET_NAME} ${SOURCE_NAMES})
target_link_libraries(${TARGET_NAME}
    PRIVATE
        imgui
        X11
        pthread
        stdc++fs
        tomopackets
        glm
        Eigen3::Eigen
        Boost::program_options
)
target_include_directories(${TARGET_NAME} PRIVATE "include")
target_compile_options(${TARGET_NAME} PRIVATE -std=c++17 -Wall -Wextra -Wfatal-errors)
# This is to work around GLM issue#754 <https://github.com/g-truc/glm/issues/754>
target_compile_definitions(${TARGET_NAME} PRIVATE -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_CTOR_INIT)

# INSTALL COMMANDS
install(TARGETS ${TARGET_NAME} imgui
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
