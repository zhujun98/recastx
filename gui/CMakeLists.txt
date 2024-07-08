# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut
# Author: Jun Zhu
#
# Distributed under the terms of the BSD 3-Clause License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
project(recastx-gui LANGUAGES C CXX)

if (CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    message(STATUS "CUDA compiler: ${CMAKE_CUDA_COMPILER}")
else()
    message(STATUS "NO CUDA support")
endif()

include(FindOpenGL)
message(STATUS "Found OpenGL at ${OPENGL_LIBRARIES}, ${OPENGL_INCLUDE_DIR}")

find_package(glm 0.9.9.8 REQUIRED)
message(STATUS "Found glm ${glm_VERSION}")

find_package(glfw3 3.4 REQUIRED)
message(STATUS "Found glfw ${glfw3_VERSION}")

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
        "src/graphics/iso_surface.cpp"
        "src/graphics/scene.cpp"
        "src/graphics/scene3d.cpp"
        "src/graphics/glyph_renderer.cpp"
        "src/graphics/graph_node.cpp"
        "src/graphics/image_buffer.cpp"
        "src/graphics/marchers.cpp"
        "src/graphics/shader_program.cpp"
        "src/graphics/slice.cpp"
        "src/graphics/texture.cpp"
        "src/graphics/viewport.cpp"
        "src/graphics/volume.cpp"
        "src/graphics/volume_slicer.cpp"
        "src/graphics/wireframe.cpp"
        "src/graphics/items/geometry_item.cpp"
        "src/graphics/items/axis_item.cpp"
        "src/graphics/items/axiscube_item.cpp"
        "src/graphics/items/graphics_item.cpp"
        "src/graphics/items/icon_item.cpp"
        "src/graphics/items/lamp_item.cpp"
        "src/graphics/items/logging_item.cpp"
        "src/graphics/items/material_item.cpp"
        "src/graphics/items/preproc_item.cpp"
        "src/graphics/items/projection_item.cpp"
        "src/graphics/items/recon_item.cpp"
        "src/graphics/items/statusbar_item.cpp"
        "src/graphics/widgets/render3d_widget.cpp"
        "src/graphics/widgets/transfer_func_widget.cpp"
        ${IMGUI_SOURCE_FILES}
        ${IMPLOT_SOURCE_FILES}
        ${GL3W_BUILD_DIR}/src/gl3w.c
        )

set(TARGET_NAME recastx-gui)
add_executable(${TARGET_NAME} ${GUI_SOURCES})
target_include_directories(${TARGET_NAME}
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_CURRENT_LIST_DIR}/../common/include
                ${CMAKE_CURRENT_LIST_DIR}/../models/include
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
                glm::glm
                ${FREETYPE_LIBRARIES}
                pthread
                Eigen3::Eigen
                Boost::program_options
                spdlog::spdlog
                recastx_grpc_proto
                recastx_models
        )

if (UNIX AND NOT APPLE)
    # required by glfw
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