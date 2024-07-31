/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Algorithms for drag-and-drop slices in 3D space are originally from https://github.com/cicwi/RECAST3D.git.
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cassert>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <implot.h>

#include "common/utils.hpp"
#include "models/cube_model.hpp"
#include "graphics/camera.hpp"
#include "graphics/primitives.hpp"
#include "graphics/slice_component.hpp"
#include "graphics/style.hpp"
#include "graphics/mesh_object.hpp"
#include "graphics/voxel_object.hpp"
#include "graphics/material_manager.hpp"
#include "logger.hpp"

namespace recastx::gui {

ReconComponent::ReconComponent() {

    glGenVertexArrays(1, &rotation_axis_vao_);
    glBindVertexArray(rotation_axis_vao_);
    glGenBuffers(1, &rotation_axis_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, rotation_axis_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::line), primitives::line, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    update_min_max_val_ = true;
}

ReconComponent::~ReconComponent() {
    glDeleteVertexArrays(1, &rotation_axis_vao_);
    glDeleteBuffers(1, &rotation_axis_vbo_);
}

void ReconComponent::preRender() {
    for (auto& [_, policy, slice] : slices_) {
        if (policy != DISABLE_SLI) {
            std::lock_guard lck(slice_mtx_);
            slice->preRender();
        }
    }

    if (volume_policy_ != DISABLE_VOL) {
        std::lock_guard lck(volume_mtx_);
        volume_->preRender();
    }

    if (update_min_max_val_ && auto_levels_) {
        updateMinMaxValues();
        update_min_max_val_ = false;
    }
}
//
//void ReconComponent::renderGl() {
//    glEnable(GL_DEPTH_TEST);
//
//    vp_->use();
//
//    cm_.bind();
//    am_.bind();
//
//    const auto& projection = vp_->projection();
//    const auto& view = scene_.cameraMatrix();
//    const auto& view_dir = scene_.cameraDir();
//    const auto& view_pos = scene_.cameraPosition();
//
//    matrix_ = projection * view;
//
//    float min_val = min_val_;
//    if (clamp_negatives_) min_val = min_val_ > 0 ? min_val_ : 0;

//    volume_->bind();
//    for (auto slice : sortedSlices()) {
//        slice->render(view, projection, min_val, max_val_,
//                      volume_->hasTexture() && volume_policy_ == PREVIEW_VOL,
//                      view_dir,
//                      view_pos);
//    }
//    volume_->unbind();

//    if (volume_policy_ == SHOW_VOL) {
//        volume_->render(view, projection, min_val, max_val_,
//                        view_dir, view_pos, vp_);
//    }
//
//    am_.unbind();
//    cm_.unbind();
//
//    if (show_wireframe_) {
//        wireframe_->render(view, projection);
//    }
//
//    glDisable(GL_DEPTH_TEST);
//
//    if (drag_machine_ != nullptr && drag_machine_->type() == DragType::rotator) {
//        auto& rotator = *(SliceRotator*)drag_machine_.get();
//        // FIXME: fix the hack
//        wireframe_->shader()->setMat4(
//                "view", view * glm::translate(rotator.rot_base) * glm::scale(rotator.rot_end - rotator.rot_base));
//        wireframe_->shader()->setVec4("color", glm::vec4(1.f, 1.f, 1.f, 1.f));
//        glBindVertexArray(rotation_axis_vao_);
//#ifndef __APPLE__
//        glLineWidth(2.f);
//#endif
//        glDrawArrays(GL_LINES, 0, 2);
//    }
//}

//

void ReconComponent::maybeSwitchDragMachine(ReconComponent::DragType type) {
    if (drag_machine_ == nullptr || drag_machine_->type() != type) {
        switch (type) {
            case DragType::translator:
                drag_machine_ = std::make_unique<SliceTranslator>(
                        *this, glm::vec2{prev_x_, prev_y_});
                break;
            case DragType::rotator:
                drag_machine_ = std::make_unique<SliceRotator>(
                        *this, glm::vec2{prev_x_, prev_y_});
                break;
            default:
                break;
        }
    }
}

} // namespace recastx::gui
