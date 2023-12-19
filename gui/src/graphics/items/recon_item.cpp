/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Algorithms for drag-and-drop slices in 3D space are originally from https://github.com/cicwi/RECAST3D.git.
 *
 * Distributed under the terms of the GPLv3 License.
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
#include "graphics/items/recon_item.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/primitives.hpp"
#include "graphics/slice.hpp"
#include "graphics/style.hpp"
#include "graphics/volume.hpp"
#include "graphics/wireframe.hpp"
#include "logger.hpp"

namespace recastx::gui {

ReconItem::ReconItem(Scene& scene)
        : GraphicsItem(scene),
          volume_(new Volume{}),
          wireframe_(new Wireframe{}) {
    scene.addItem(this);

    glGenVertexArrays(1, &rotation_axis_vao_);
    glBindVertexArray(rotation_axis_vao_);
    glGenBuffers(1, &rotation_axis_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, rotation_axis_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::line), primitives::line, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    slices_.emplace_back(0, SHOW2D_SLI, std::make_unique<Slice>(0, Slice::Plane::YZ));
    slices_.emplace_back(1, SHOW2D_SLI, std::make_unique<Slice>(1, Slice::Plane::XZ));
    slices_.emplace_back(2, SHOW2D_SLI, std::make_unique<Slice>(2, Slice::Plane::XY));
    assert(slices_.size() == MAX_NUM_SLICES);
    update_min_max_val_ = true;
}

ReconItem::~ReconItem() {
    glDeleteVertexArrays(1, &rotation_axis_vao_);
    glDeleteBuffers(1, &rotation_axis_vbo_);
}

void ReconItem::onWindowSizeChanged(int width, int height) {
    st_win_size_ = {
        (1.f - Style::ICON_WIDTH - 4 * Style::MARGIN) * (float)width
            - Style::TOP_PANEL_HEIGHT * (float)height,
        Style::TOP_PANEL_HEIGHT * (float)height
    };
    st_win_pos_ = {
        (2.f * Style::MARGIN + Style::ICON_WIDTH) * (float)width,
        Style::MARGIN * (float)height
    };
}

void ReconItem::renderIm() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "RECONSTRUCTION");

    auto& cmd = Colormap::data();
    if (ImGui::BeginCombo("Colormap##RECON", cmd.GetName(cm_.get()))) {
        for (auto idx : Colormap::options()) {
            const char* name = cmd.GetName(idx);
            if (ImGui::Selectable(name, cm_.get() == idx)) {
                cm_.set(idx);
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Checkbox("Auto levels", &auto_levels_) && auto_levels_) {
        update_min_max_val_ = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Clamp negatives", &clamp_negatives_);

    ImGui::BeginDisabled(auto_levels_);
    float step_size = (max_val_ - min_val_) / 100.f;
    if (step_size < 0.01f) step_size = 0.01f; // avoid a tiny step size
    ImGui::DragFloatRange2("Min / Max", &min_val_, &max_val_, step_size,
                           std::numeric_limits<float>::lowest(), // min() does not work
                           std::numeric_limits<float>::max());
    ImGui::EndDisabled();

    renderImSliceControl<0>("Slice 1##RECON");
    renderImSliceControl<1>("Slice 2##RECON");
    renderImSliceControl<2>("Slice 3##RECON");

    if (ImGui::Button("Reset all slices")) {
        for (auto& slice : slices_) {
            std::get<2>(slice)->reset();
        }
        updateServerSliceParams();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Show slice histograms", &show_statistics_);
    if (show_statistics_) {
        ImGui::SetNextWindowPos(st_win_pos_);
        ImGui::SetNextWindowSize(st_win_size_);

        ImGui::Begin("Statistics##ReconItem", NULL, ImGuiWindowFlags_NoDecoration);

        ImPlot::BeginSubplots("##Histograms", 1, MAX_NUM_SLICES, ImVec2(-1.f, -1.f));
        for (auto& [_, policy, slice]: slices_) {
            const auto &data = slice->data();
            // FIXME: faster way to build the title?
            if (ImPlot::BeginPlot(("Slice " + std::to_string(slice->id())).c_str(),
                                  ImVec2(-1.f, -1.f))) {
                ImPlot::SetupAxes("Pixel value", "Density",
                                  ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                if (policy != DISABLE_SLI) {
                    ImPlot::PlotHistogram("##Histogram", data.data(), static_cast<int>(data.size()),
                                          100, false, true);
                }
                ImPlot::EndPlot();
            }
        }
        ImPlot::EndSubplots();

        ImGui::End();
    }

    renderImVolumeControl();

    ImGui::Checkbox("Show wireframe##RECON", &show_wireframe_);

    ImGui::Separator();

    scene_.setStatus("volumeUpdateFrameRate", fps_counter_.volumeFrameRate());
    scene_.setStatus("sliceUpdateFrameRate", fps_counter_.sliceFrameRate());
}

void ReconItem::onFramebufferSizeChanged(int /* width */, int /* height */) {}

void ReconItem::preRenderGl() {
    for (auto& [_, policy, slice] : slices_) {
        if (policy != DISABLE_SLI) slice->preRender();
    }
    if (volume_policy_ != DISABLE_VOL) volume_->preRender();

    if (update_min_max_val_ && auto_levels_) {
        updateMinMaxValues();
        update_min_max_val_ = false;
    }
}

void ReconItem::renderGl() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    cm_.bind();

    const auto& projection = scene_.projectionMatrix();
    const auto& view = scene_.viewMatrix();

    matrix_ = projection * view;

    float min_val = min_val_;
    if (clamp_negatives_) min_val = min_val_ > 0 ? min_val_ : 0;

    volume_->bind();
    for (auto slice : sortedSlices()) {
        slice->render(view, projection, min_val, max_val_,
                      !volume_->empty() && volume_policy_ == PREVIEW_VOL);
    }
    volume_->unbind();

    if (volume_policy_ == SHOW_VOL) {
        volume_->render(view, projection, min_val, max_val_, volume_alpha_);
    }

    cm_.unbind();

    if (show_wireframe_) {
        wireframe_->render(view, projection);
    }

    glDisable(GL_DEPTH_TEST);

    if (drag_machine_ != nullptr && drag_machine_->type() == DragType::rotator) {
        auto& rotator = *(SliceRotator*)drag_machine_.get();
        // FIXME: fix the hack
        wireframe_->shader()->setMat4(
                "view", view * glm::translate(rotator.rot_base) * glm::scale(rotator.rot_end - rotator.rot_base));
        wireframe_->shader()->setVec4("color", glm::vec4(1.f, 1.f, 1.f, 1.f));
        glBindVertexArray(rotation_axis_vao_);
        glLineWidth(10.f);
        glDrawArrays(GL_LINES, 0, 2);
    }

    glDisable(GL_BLEND);
}

bool ReconItem::updateServerParams() {
    return updateServerSliceParams() || updateServerVolumeParams();
}

void ReconItem::setSliceData(const std::string& data,
                             const std::array<uint32_t, 2>& size,
                             uint64_t timestamp) {
    size_t sid = sliceIdFromTimestamp(timestamp);
    auto& slice = slices_[sid];
    if (std::get<0>(slice) == timestamp) {
        Slice* ptr = std::get<2>(slice).get();
        if (ptr == dragged_slice_) return;

        Slice::DataType slice_data(size[0] * size[1]);
        std::memcpy(slice_data.data(), data.data(), data.size());
        assert(data.size() == slice_data.size() * sizeof(Slice::DataType::value_type));
        ptr->setData(std::move(slice_data), {size[0], size[1]});
        log::info("Set slice data {}", sid);
        update_min_max_val_ = true;
    } else {
        // Ignore outdated reconstructed slices.
        log::debug("Outdated slice received: {} ({})", sid, timestamp);
    }
}

void ReconItem::setVolumeData(const std::string& data, const std::array<uint32_t, 3>& size) {
    Volume::DataType volume_data(size[0] * size[1] * size[2]);
    std::memcpy(volume_data.data(), data.data(), data.size());
    assert(data.size() == volume_data.size() * sizeof(Volume::DataType::value_type));
    volume_->setData(std::move(volume_data), {size[0], size[1], size[2]});
    log::info("Set volume data");
    update_min_max_val_ = true;
}

bool ReconItem::consume(const DataType& packet) {
    if (std::holds_alternative<rpc::ReconData>(packet)) {
        const auto& data = std::get<rpc::ReconData>(packet);
        if (data.has_slice()) {
            auto& slice = data.slice();
            setSliceData(slice.data(), {slice.row_count(), slice.col_count()}, slice.timestamp());
            fps_counter_.countSlice();
            return true;
        }

        if (volume_policy_ != DISABLE_VOL && data.has_volume()) {
            auto& volume = data.volume();
            setVolumeData(volume.data(), {volume.row_count(), volume.col_count(), volume.slice_count()});
            fps_counter_.countVolume();
            return true;
        }
    }

    return false;
}

bool ReconItem::handleMouseButton(int button, int action) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (hovered_slice_ != nullptr) {
                maybeSwitchDragMachine(DragType::translator);
                dragged_slice_ = hovered_slice_;

                log::debug("Set dragged slice: {}", dragged_slice_->id());

                return true;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (hovered_slice_ != nullptr) {
                maybeSwitchDragMachine(DragType::rotator);
                dragged_slice_ = hovered_slice_;

                log::debug("Set dragged slice: {}", dragged_slice_->id());

                return true;
            }
        }
    } else if (action == GLFW_RELEASE) {
        if (dragged_slice_ != nullptr) {
            std::get<0>(slices_[dragged_slice_->id()]) += MAX_NUM_SLICES;
            scene_.client()->setSlice(std::get<0>(slices_[dragged_slice_->id()]),
                                      dragged_slice_->orientation3());

            log::debug("Sent slice {} ({}) orientation update request",
                       dragged_slice_->id(), std::get<0>(slices_[dragged_slice_->id()]));

            dragged_slice_ = nullptr;
            drag_machine_ = nullptr;
            return true;
        }
        drag_machine_ = nullptr;
    }

    return false;
}

bool ReconItem::handleMouseMoved(float x, float y) {
    // update slice that is being hovered over
    y = -y;

    if (prev_y_ < -1.0f) {
        prev_x_ = x;
        prev_y_ = y;
    }

    glm::vec2 delta(x - prev_x_, y - prev_y_);
    prev_x_ = x;
    prev_y_ = y;

    // TODO: fix for screen ratio
    if (dragged_slice_ != nullptr) {
        dragged_slice_->setEmpty();
        update_min_max_val_ = true;

        drag_machine_->onDrag(delta);
        return true;
    }

    updateHoveringSlice(x, y);

    return false;
}

template<size_t index>
void ReconItem::renderImSliceControl(const char* header) {

    static const char* BTN_2D[] = {"2D##RECON_SLI_0", "2D##RECON_SLI_1", "2D##RECON_SLI_2"};
    static const char* BTN_3D[] = {"3D##RECON_SLI_0", "3D##RECON_SLI_1", "3D##RECON_SLI_2"};
    static const char* BTN_DISABLE[] = {"Disable##RECON_SLI_0", "Disable##RECON_SLI_1", "Disable##RECON_SLI_2"};
    static const char* COMBO[] = {"Orientation##RECON_PLANE_0", "Orientation##RECON_PLANE_1", "Orientation##RECON_PLANE_2"};
    static const ImVec4 K_HEADER_COLOR = (ImVec4)ImColor(65, 145, 151);

    auto& [timestamp, _, slice] = slices_[index];

    ImGui::PushStyleColor(ImGuiCol_Header, K_HEADER_COLOR);
    bool expand = ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen);
    slice->setHighlighted(ImGui::IsItemHovered());

    ImGui::PopStyleColor();
    if (expand) {
        bool cd = false;

        cd |= ImGui::RadioButton(BTN_2D[index], &std::get<1>(slices_[index]), SHOW2D_SLI);
        ImGui::SameLine();
        cd |= ImGui::RadioButton(BTN_3D[index], &std::get<1>(slices_[index]), SHOW3D_SLI);
        ImGui::SameLine();
        cd |= ImGui::RadioButton(BTN_DISABLE[index], &std::get<1>(slices_[index]), DISABLE_SLI);

        if (cd) update_min_max_val_ = true;

        static const std::map<Slice::Plane, std::string> plane_options {
                {Slice::Plane::XY, "X-Y"},
                {Slice::Plane::YZ, "Y-Z"},
                {Slice::Plane::XZ, "X-Z"},
        };

        ImGui::AlignTextToFramePadding();
        ImGui::PushItemWidth(60);
        Slice::Plane curr_plane = slice->plane();
        if (ImGui::BeginCombo(COMBO[index], plane_options.at(curr_plane).c_str())) {
            for (auto& [k, v] : plane_options) {
                if (ImGui::Selectable(v.c_str(), curr_plane == k)) {
                    slice->setPlane(k);
                    update_min_max_val_ = true;
                    scene_.client()->setSlice(timestamp, slice->orientation3());
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
    }
}

void ReconItem::renderImVolumeControl() {
    static const ImVec4 K_HEADER_COLOR = (ImVec4)ImColor(65, 145, 151);
    ImGui::PushStyleColor(ImGuiCol_Header, K_HEADER_COLOR);
    bool expand = ImGui::CollapsingHeader("Volume##RECON", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();

    if (expand) {
        bool cd = false;
        cd |= ImGui::RadioButton("Preview##RECON_VOL", &volume_policy_, PREVIEW_VOL);
        ImGui::SameLine();
        cd |= ImGui::RadioButton("Show##RECON_VOL", &volume_policy_, SHOW_VOL);
        ImGui::SameLine();
        cd |= ImGui::RadioButton("Disable##RECON_VOL", &volume_policy_, DISABLE_VOL);

        if (cd) {
            update_min_max_val_ = true;
            updateServerVolumeParams();
        }

        ImGui::SliderFloat("Alpha##RECON_VOL", &volume_alpha_, 0.0f, 1.0f);
    }
}

bool ReconItem::updateServerSliceParams() {
    for (auto& slice : slices_) {
        if (scene_.client()->setSlice(std::get<0>(slice), std::get<2>(slice)->orientation3())) {
            return true;
        }
    }
    return false;
}

bool ReconItem::updateServerVolumeParams() {
    return scene_.client()->setVolume(volume_policy_ != DISABLE_VOL);
}

void ReconItem::updateHoveringSlice(float x, float y) {
    auto inv_matrix = glm::inverse(matrix_);
    Slice* matched_slice = nullptr;
    float best_z = std::numeric_limits<float>::max();
    for (auto& [_, policy, slice] : slices_) {
        if (policy == DISABLE_SLI) continue;
        slice->setHovered(false);
        auto maybe_point = intersectionPoint(inv_matrix, slice->orientation4(), glm::vec2(x, y));
        if (std::get<0>(maybe_point)) {
            auto z = std::get<1>(maybe_point);
            if (z < best_z) {
                best_z = z;
                matched_slice = slice.get();
            }
        }
    }

    if (matched_slice != nullptr) matched_slice->setHovered(true);
    hovered_slice_ = matched_slice;
}

std::vector<Slice*> ReconItem::sortedSlices() const {
    std::vector<Slice*> sorted;
    for (auto& [_, policy, slice] : slices_) {
        if (policy == DISABLE_SLI) continue;
        sorted.push_back(slice.get());
    }
    std::sort(sorted.begin(), sorted.end(), [](auto& lhs, auto& rhs) -> bool {
        if (rhs->transparent() == lhs->transparent()) {
            return rhs->id() < lhs->id();
        }
        return rhs->transparent();
    });
    return sorted;
}

void ReconItem::maybeSwitchDragMachine(ReconItem::DragType type) {
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

void ReconItem::updateMinMaxValues() {
    min_val_ = std::numeric_limits<float>::max();
    max_val_ = std::numeric_limits<float>::min();
    bool has_value = false;

    for (auto& [_, policy, slice] : slices_) {
        if (policy == DISABLE_SLI) continue;

        auto& min_max_v = slice->minMaxVals();
        if (min_max_v) {
            auto [min_v, max_v] = min_max_v.value();
            min_val_ = min_v < min_val_ ? min_v : min_val_;
            max_val_ = max_v > max_val_ ? max_v : max_val_;
            has_value = true;
        }

    }

    if (volume_policy_ != DISABLE_VOL) {
        auto& min_max_v = volume_->minMaxVals();
        if (min_max_v) {
            auto [min_v, max_v] = min_max_v.value();
            min_val_ = min_v < min_val_ ? min_v : min_val_;
            max_val_ = max_v > max_val_ ? max_v : max_val_;
            has_value = true;
        }
    }

    if (!has_value) {
        min_val_ = 0.f;
        max_val_ = 0.f;
    }
}

// ReconItem::DragMachine

ReconItem::DragMachine::DragMachine(ReconItem& comp, const glm::vec2& initial, DragType type)
    : comp_(comp), initial_(initial), type_(type) {}

ReconItem::DragMachine::~DragMachine() = default;

// ReconItem::SliceTranslator

ReconItem::SliceTranslator::SliceTranslator(ReconItem &comp, const glm::vec2& initial)
    : DragMachine(comp, initial, DragType::translator) {}

ReconItem::SliceTranslator::~SliceTranslator() = default;

void ReconItem::SliceTranslator::onDrag(const glm::vec2& delta) {
    Slice* slice = comp_.draggedSlice();
    auto& o = slice->orientation4();

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto normal = glm::normalize(glm::cross(axis1, axis2));

    // project the normal vector to screen coordinates
    // FIXME maybe need window matrix here too which would be kind of
    // painful maybe
    auto base_point_normal = glm::vec3(o[2][0], o[2][1], o[2][2]) + 0.5f * (axis1 + axis2);
    auto end_point_normal = base_point_normal + normal;

    auto a = comp_.matrix_ * glm::vec4(base_point_normal, 1.0f);
    auto b = comp_.matrix_ * glm::vec4(end_point_normal, 1.0f);
    auto normal_delta = b - a;
    float difference = glm::dot(glm::vec2(normal_delta.x, normal_delta.y), delta);

    // take the inner product of delta x and this normal vector

    auto dx = difference * normal;
    // FIXME check if it is still inside the bounding box of the volume
    // probably by checking all four corners are inside bounding box, should
    // define this box somewhere
    o[2][0] += dx[0];
    o[2][1] += dx[1];
    o[2][2] += dx[2];
}

// ReconItem::SliceRotator

ReconItem::SliceRotator::SliceRotator(ReconItem& comp, const glm::vec2& initial)
    : DragMachine(comp, initial, DragType::rotator) {
    // 1. need to identify the opposite axis
    // a) get the position within the slice
    auto tf = comp.matrix_;
    auto inv_matrix = glm::inverse(tf);

    Slice* slice = comp.hoveredSlice();
    assert(slice != nullptr);
    auto o = slice->orientation4();

    auto maybe_point = intersectionPoint(inv_matrix, o, initial_);
    assert(std::get<0>(maybe_point));

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);

    auto in_world = std::get<2>(maybe_point);
    auto rel = in_world - base;

    auto x = 0.5f * glm::dot(rel, axis1) - 1.0f;
    auto y = 0.5f * glm::dot(rel, axis2) - 1.0f;

    // 2. need to rotate around that at on drag
    auto other = glm::vec3();
    if (glm::abs(x) > glm::abs(y)) {
        if (x > 0.0f) {
            rot_base = base;
            rot_end = rot_base + axis2;
            other = axis1;
        } else {
            rot_base = base + axis1;
            rot_end = rot_base + axis2;
            other = -axis1;
        }
    } else {
        if (y > 0.0f) {
            rot_base = base;
            rot_end = rot_base + axis1;
            other = axis2;
        } else {
            rot_base = base + axis2;
            rot_end = rot_base + axis1;
            other = -axis2;
        }
    }

    auto center = 0.5f * (rot_end + rot_base);
    auto opposite_center = 0.5f * (rot_end + rot_base) + other;
    auto from = tf * glm::vec4(glm::rotate(rot_base - center,
                                           glm::half_pi<float>(), other) +
                                   opposite_center,
                               1.0f);
    auto to = tf * glm::vec4(glm::rotate(rot_end - center,
                                         glm::half_pi<float>(), other) +
                                 opposite_center,
                             1.0f);

    screen_direction = glm::normalize(from - to);
}

ReconItem::SliceRotator::~SliceRotator() = default;

void ReconItem::SliceRotator::onDrag(const glm::vec2& delta) {
    Slice* slice = comp_.draggedSlice();
    auto& o = slice->orientation4();

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);

    auto a = base - rot_base;
    auto b = base + axis1 - rot_base;
    auto c = base + axis2 - rot_base;

    auto weight = glm::dot(delta, screen_direction);
    a = glm::rotate(a, weight, rot_end - rot_base) + rot_base;
    b = glm::rotate(b, weight, rot_end - rot_base) + rot_base;
    c = glm::rotate(c, weight, rot_end - rot_base) + rot_base;

    slice->setOrientation(a, b - a, c - a);
}

} // namespace recastx::gui
