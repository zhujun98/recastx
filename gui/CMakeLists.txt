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
        "src/main.cpp"
        "src/rpc_client.cpp"
        "src/utils.cpp"
        "src/graphics/aesthetics.cpp"
        "src/graphics/camera.cpp"
        "src/graphics/geometry_component.cpp"
        "src/graphics/glyph_object.cpp"
        "src/graphics/glyph_renderer.cpp"
        "src/graphics/image_object.cpp"
        "src/graphics/light_manager.cpp"
        "src/graphics/light_object.cpp"
        "src/graphics/log_component.cpp"
        "src/graphics/marcher.cpp"
        "src/graphics/material_manager.cpp"
        "src/graphics/mesh.cpp"
        "src/graphics/mesh_object.cpp"
        "src/graphics/preproc_component.cpp"
        "src/graphics/projection_component.cpp"
        "src/graphics/renderer.cpp"
        "src/graphics/scan_component.cpp"
        "src/graphics/scene.cpp"
        "src/graphics/shader_program.cpp"
        "src/graphics/simple_object.cpp"
        "src/graphics/slice_component.cpp"
        "src/graphics/slice_object.cpp"
        "src/graphics/volume_component.cpp"
        "src/graphics/volume_slicer.cpp"
        "src/graphics/voxel_object.cpp"
        "src/graphics/widgets/light_widget.cpp"
        "src/graphics/widgets/material_widget.cpp"
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