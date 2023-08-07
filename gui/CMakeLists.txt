# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut PSI
# Author: Jun Zhu
#
# Distributed under the terms of the GPLv3 License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
project(recastx-gui LANGUAGES C CXX)

include(FindOpenGL)
message(STATUS "Found OpenGL at ${OPENGL_LIBRARIES}, ${OPENGL_INCLUDE_DIR}")

find_package(Eigen3 3.4 REQUIRED NO_MODULE)
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

# freetype
find_package(Freetype 2.12 REQUIRED)

# imgui
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/../ext/imgui")
set(IMGUI_SOURCE_FILES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        )

#implot
set(IMPLOT_DIR "${PROJECT_SOURCE_DIR}/../ext/implot")
set(IMPLOT_SOURCE_FILES
        ${IMPLOT_DIR}/implot.cpp
        ${IMPLOT_DIR}/implot_items.cpp)

# build
set(GUI_SOURCES
        "src/application.cpp"
        "src/rpc_client.cpp"
        "src/main.cpp"
        "src/math_common.cpp"
        "src/path.cpp"
        "src/utils.cpp"
        "src/graphics/aesthetics.cpp"
        "src/graphics/camera3d.cpp"
        "src/graphics/scene.cpp"
        "src/graphics/scene3d.cpp"
        "src/graphics/framebuffer.cpp"
        "src/graphics/glyph_renderer.cpp"
        "src/graphics/graph_node.cpp"
        "src/graphics/projection.cpp"
        "src/graphics/shader_program.cpp"
        "src/graphics/slice.cpp"
        "src/graphics/viewport.cpp"
        "src/graphics/volume.cpp"
        "src/graphics/items/axes_item.cpp"
        "src/graphics/items/axiscube_item.cpp"
        "src/graphics/items/graphics_item.cpp"
        "src/graphics/items/icon_item.cpp"
        "src/graphics/items/logging_item.cpp"
        "src/graphics/items/preproc_item.cpp"
        "src/graphics/items/projection_item.cpp"
        "src/graphics/items/recon_item.cpp"
        "src/graphics/items/statusbar_item.cpp"
        ${IMGUI_SOURCE_FILES}
        ${IMPLOT_SOURCE_FILES}
        ${GL3W_BUILD_DIR}/src/gl3w.c
        )

set(TARGET_NAME recastx-gui)
add_executable(${TARGET_NAME} ${GUI_SOURCES})
target_include_directories(${TARGET_NAME}
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_CURRENT_LIST_DIR}/../common/include
                ${FREETYPE_INCLUDE_DIRS}
                ${IMGUI_DIR}
                ${IMGUI_DIR}/backends
                ${IMPLOT_DIR}
                ${GL3W_BUILD_DIR}/include
)
target_link_libraries(${TARGET_NAME}
        PRIVATE dl
                glfw
                OpenGL::GL
                glm
                ${FREETYPE_LIBRARIES}
                pthread
                Eigen3::Eigen
                Boost::program_options
                spdlog::spdlog
                recastx_grpc_proto
        )

if (NOT APPLE)
    target_link_libraries(${TARGET_NAME} PRIVATE X11)
endif()
target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wfatal-errors)
# This is to work around GLM issue#754 <https://github.com/g-truc/glm/issues/754>
target_compile_definitions(${TARGET_NAME}
        PUBLIC -DVERBOSITY=${VERBOSITY}
        PRIVATE -DGLM_ENABLE_EXPERIMENTAL
                -DGLM_FORCE_CTOR_INIT
        )

if (BUILD_TEST)
    add_subdirectory(tests)
endif()

# INSTALL COMMANDS
install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION bin)