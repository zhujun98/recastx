/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "graphics/slice_component.hpp"
#include "graphics/volume_component.hpp"
#include "graphics/slice_object.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/widgets/widget.hpp"
#include "common/utils.hpp"

namespace recastx::gui {

SliceComponent::SliceComponent(RpcClient* client) : Component(client) {}

SliceComponent::~SliceComponent() = default;

void SliceComponent::addSliceObject(SliceObject* obj) {
    static std::array<Plane, MAX_NUM_SLICES> planes { Plane::YZ, Plane::XZ, Plane::XY };

    Slice slice;
    slice.id = slices_.size();
    slice.timestamp = slice.id;
    slice.object = obj;
    slice.plane = planes[slices_.size()];
    reset(slice);

    slices_.push_back(std::move(slice));
    assert(slices_.size() <= MAX_NUM_SLICES);
}

void SliceComponent::preRender() {
    std::lock_guard lk(mtx_);

    for (auto& slice : slices_) {
        if (slice.update_texture) {
            if (slice.data.empty()) {
                slice.object->resetIntensity();
            } else {
                slice.object->setIntensity(slice.data.data(), slice.data.x(), slice.data.y());

                auto mat = MaterialManager::instance().getMaterial<TransferFunc>(slice.object->materialID());
                const auto& v = slice.data.minMaxVals();
                if (v) mat->registerMinMaxVals(v.value());
            }
            slice.update_texture = false;
        }
    }
}

void SliceComponent::draw(rpc::ServerState_State) {
    renderSliceControl(0, "Slice 1##RECON");
    renderSliceControl(1, "Slice 2##RECON");
    renderSliceControl(2, "Slice 3##RECON");

    if (ImGui::Button("Reset all slices##RECON")) {
        for (auto& slice : slices_) reset(slice);
        updateServerParams();
    }

    for (auto& slice : slices_) {
        if (slice.object->visible()) {
            MaterialManager::instance().getWidget(slice.object->materialID())->draw();
            break;
        }
    }
}

void SliceComponent::drawStatistics(rpc::ServerState_State) {
    ImPlot::BeginSubplots("##Histograms", 1, MAX_NUM_SLICES, ImVec2(-1.f, -1.f));
    std::lock_guard lk(mtx_);
    for (auto& slice: slices_) {
        if (ImPlot::BeginPlot(("Slice " + std::to_string(slice.id)).c_str(),
                              ImVec2(-1.f, -1.f))) {
            ImPlot::SetupAxes("Pixel value", "Density",
                              ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            if (slice.object->visible()) {
                ImPlot::PlotHistogram("##Histogram",
                                      slice.data.data(),
                                      static_cast<int>(slice.data.size()),
                                      100,
                                      1.0);
            }
            ImPlot::EndPlot();
        }
    }
    ImPlot::EndSubplots();
}

RpcClient::State SliceComponent::updateServerParams() const {
    for (auto& slice : slices_) {
        CHECK_CLIENT_STATE(client_->setSlice(slice.timestamp, slice.object->orientation()))
    }
    return RpcClient::State::OK;
}

bool SliceComponent::setData(size_t timestamp, const std::string& data, uint32_t x, uint32_t y) {
    size_t sid = sliceIdFromTimestamp(timestamp);
    auto& slice = slices_[sid];

    if (slice.object->dragging()) return false;

    if (slice.timestamp == timestamp) {
        {
            std::lock_guard lck(mtx_);
            slice.data.setData(data, x, y);
            slice.update_texture = true;
        }

        log::info("New slice {} data is ready: {} x {}", sid, x, y);
        return true;
    }

    log::debug("Outdated slice received: {} ({})", sid, timestamp);
    return false;
}

void SliceComponent::onShowVolume() {
    for (auto& slice : slices_) {
        slice.display_policy = DISABLE;
    }
}

void SliceComponent::setSampleVolume(bool state) {
    for (auto& slice : slices_) {
        slice.object->setSampleVolume(state);
    }
}

void SliceComponent::renderSliceControl(size_t index, const char* header) {
    static const char* BTN_SHOW[] = {"Show##SLICE_COMP_0", "Show##SLICE_COMP_1", "Show##SLICE_COMP_2"};
    static const char* BTN_DISABLE[] = {"Disable##SLICE_COMP_0", "Disable##SLICE_COMP_1", "Disable##SLICE_COMP_2"};
    static const char* COMBO[] = {"##SLICE_COMP_PLANE_0", "##SLICE_COMP_PLANE_1", "##SLICE_COMP_PLANE_2"};
    static const char* TRANSLATION[] = {"Translation##SLICE_COMP_0", "Translation##SLICE_COMP_1", "Translation##SLICE_COMP_2"};

    auto& slice = slices_[index];

    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    bool expand = ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen);
    bool hovering = ImGui::IsItemHovered();

    ImGui::PopStyleColor();
    if (expand) {
        static const std::map<Plane, std::string> plane_options {
                {Plane::XY, "X-Y"},
                {Plane::YZ, "Y-Z"},
                {Plane::XZ, "X-Z"},
        };

        ImGui::AlignTextToFramePadding();
        ImGui::PushItemWidth(60);
        Plane curr_plane = slice.plane;
        if (ImGui::BeginCombo(COMBO[index], plane_options.at(curr_plane).c_str())) {
            for (auto& [k, v] : plane_options) {
                if (ImGui::Selectable(v.c_str(), curr_plane == k)) {
                    slice.plane = k;
                    reset(slice);
                    slice.timestamp += MAX_NUM_SLICES;
                    client_->setSlice(slice.timestamp, slice.object->orientation());
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        bool cd = false;
        cd |= ImGui::RadioButton(BTN_SHOW[index], &slice.display_policy, SHOW);
        ImGui::SameLine();
        cd |= ImGui::RadioButton(BTN_DISABLE[index], &slice.display_policy, DISABLE);
        slice.object->setVisible(slice.display_policy == SHOW);
        if (cd && slice.display_policy == SHOW) {
            volume_comp_->onShowSlice();
        }

        {
            auto ret = ImGui::SliderFloat(TRANSLATION[index], &slice.offset, -1.f, 1.f, "%.3f",
                                          ImGuiSliderFlags_AlwaysClamp);
            hovering |= ImGui::IsItemHovered();

            if (ret) {
                slice.object->setOffset(slice.offset);
            }

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                slice.timestamp += MAX_NUM_SLICES;
                log::warn("Update slice orientation");
                client_->setSlice(slice.timestamp, slice.object->orientation());
            }

            slice.object->setDragging(ret);
        }
    }

    slice.object->setHovering(hovering);
}

void SliceComponent::reset(Slice& slice) {
    if (slice.plane == Plane::YZ) {
        slice.object->setOrientation(glm::vec3( 0.0f, -1.0f, -1.0f),
                                     glm::vec3( 0.0f,  2.0f,  0.0f),
                                     glm::vec3( 0.0f,  0.0f,  2.0f));
    } else if (slice.plane == Plane::XZ) {
        slice.object->setOrientation(glm::vec3(-1.0f,  0.0f, -1.0f),
                                     glm::vec3( 2.0f,  0.0f,  0.0f),
                                     glm::vec3( 0.0f,  0.0f,  2.0f));
    } else if (slice.plane == Plane::XY) {
        slice.object->setOrientation(glm::vec3(-1.0f, -1.0f,  0.0f),
                                     glm::vec3( 2.0f,  0.0f,  0.0f),
                                     glm::vec3( 0.0f,  2.0f,  0.0f));
    }
}

//std::vector<Slice*> ReconComponent::sortedSlices() const {
//    std::vector<Slice*> sorted;
//    for (auto& [_, policy, slice] : slices_) {
//        if (policy == DISABLE_SLI) continue;
//        sorted.push_back(slice.get());
//    }
//    std::sort(sorted.begin(), sorted.end(), [](auto& lhs, auto& rhs) -> bool {
//        if (rhs->transparent() == lhs->transparent()) {
//            return rhs->id() < lhs->id();
//        }
//        return rhs->transparent();
//    });
//    return sorted;
//}
//
//bool SliceComponent::handleMouseButton(int button, int action) {
//
//    if (action == GLFW_PRESS) {
//        if (button == GLFW_MOUSE_BUTTON_LEFT) {
//            if (hovered_slice_ >= 0) {
//                maybeSwitchDragMachine(DragType::translator);
//                dragged_slice_ = hovered_slice_;
//
//                log::debug("Set dragged slice: {}", dragged_slice_);
//
//                return true;
//            }
//        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
//            if (hovered_slice_ >= 0) {
//                maybeSwitchDragMachine(DragType::rotator);
//                dragged_slice_ = hovered_slice_;
//
//                log::debug("Set dragged slice: {}", dragged_slice_);
//
//                return true;
//            }
//        }
//    } else if (action == GLFW_RELEASE) {
//        if (dragged_slice_ != nullptr) {
//            std::get<0>(slices_[dragged_slice_->id()]) += MAX_NUM_SLICES;
//            scene_.client()->setSlice(std::get<0>(slices_[dragged_slice_->id()]),
//                                      dragged_slice_->orientation3());
//
//            log::debug("Sent slice {} ({}) orientation update request",
//                       dragged_slice_->id(), std::get<0>(slices_[dragged_slice_->id()]));
//
//            dragged_slice_ = -1;
//            drag_machine_ = nullptr;
//            return true;
//        }
//        drag_machine_ = nullptr;
//    }
//
//    return false;
//}

//bool SliceComponent::handleMouseMoved(glm::vec2 delta) {
    // TODO: fix for screen ratio
//    if (dragged_slice_ >= 0) {
//        dragged_slice_->clear();
//        update_min_max_val_ = true;
//
//        drag_machine_->onDrag(delta);
//        return true;
//    }
//
//    updateHoveringSlice(x, y);
//}

//void SliceComponent::updateHoveringSlice(float x, float y) {
//    auto inv_matrix = glm::inverse(matrix_);
//    Slice* matched_slice = nullptr;
//    float best_z = std::numeric_limits<float>::max();
//    for (auto& [_, policy, slice] : slices_) {
//        if (policy == DISABLE_SLI) continue;
//        slice->setHovered(false);
//        auto maybe_point = intersectionPoint(inv_matrix, slice->orientation4(), glm::vec2(x, y));
//        if (std::get<0>(maybe_point)) {
//            auto z = std::get<1>(maybe_point);
//            if (z < best_z) {
//                best_z = z;
//                matched_slice = slice.get();
//            }
//        }
//    }
//
//    if (matched_slice != nullptr) matched_slice->setHovered(true);
//    hovered_slice_ = matched_slice;
//}



// ReconItem::DragMachine

//DragMachine::DragMachine(ReconComponent& comp, const glm::vec2& initial, DragType type)
//        : comp_(comp), initial_(initial), type_(type) {}
//
//DragMachine::~DragMachine() = default;
//
//// ReconItem::SliceTranslator
//
//SliceTranslator::SliceTranslator(ReconComponent &comp, const glm::vec2& initial)
//        : DragMachine(comp, initial, DragType::translator) {}
//
//SliceTranslator::~SliceTranslator() = default;
//
//void SliceTranslator::onDrag(const glm::vec2& delta) {
//    Slice* slice = comp_.draggedSlice();
//    const auto& o = slice->orientation4();
//
//    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
//    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
//    const auto& normal = slice->normal();
//
//    // project the normal vector to screen coordinates
//    // FIXME maybe need window matrix here too which would be kind of
//    // painful maybe
//    auto base_point_normal = glm::vec3(o[2][0], o[2][1], o[2][2]) + 0.5f * (axis1 + axis2);
//    auto end_point_normal = base_point_normal + normal;
//
//    auto a = comp_.matrix_ * glm::vec4(base_point_normal, 1.0f);
//    auto b = comp_.matrix_ * glm::vec4(end_point_normal, 1.0f);
//    auto normal_delta = b - a;
//    float difference = glm::dot(glm::vec2(normal_delta.x, normal_delta.y), delta);
//
//    // take the inner product of delta x and this normal vector
//
//    auto dx = difference * normal;
//    // FIXME check if it is still inside the bounding box of the volume
//    // probably by checking all four corners are inside bounding box, should
//    // define this box somewhere
//    slice->translate(dx);
//}

// ReconItem::SliceRotator
//
//SliceRotator::SliceRotator(ReconComponent& comp, const glm::vec2& initial)
//        : DragMachine(comp, initial, DragType::rotator) {
//    // 1. need to identify the opposite axis
//    // a) get the position within the slice
//    auto tf = comp.matrix_;
//    auto inv_matrix = glm::inverse(tf);
//
//    Slice* slice = comp.hoveredSlice();
//    assert(slice != nullptr);
//    const auto& o = slice->orientation4();
//
//    auto maybe_point = intersectionPoint(inv_matrix, o, initial_);
//    assert(std::get<0>(maybe_point));
//
//    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
//    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
//    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);
//
//    auto in_world = std::get<2>(maybe_point);
//    auto rel = in_world - base;
//
//    auto x = 0.5f * glm::dot(rel, axis1) - 1.0f;
//    auto y = 0.5f * glm::dot(rel, axis2) - 1.0f;
//
//    // 2. need to rotate around that at on drag
//    auto other = glm::vec3();
//    if (glm::abs(x) > glm::abs(y)) {
//        if (x > 0.0f) {
//            rot_base = base;
//            rot_end = rot_base + axis2;
//            other = axis1;
//        } else {
//            rot_base = base + axis1;
//            rot_end = rot_base + axis2;
//            other = -axis1;
//        }
//    } else {
//        if (y > 0.0f) {
//            rot_base = base;
//            rot_end = rot_base + axis1;
//            other = axis2;
//        } else {
//            rot_base = base + axis2;
//            rot_end = rot_base + axis1;
//            other = -axis2;
//        }
//    }
//
//    auto center = 0.5f * (rot_end + rot_base);
//    auto opposite_center = 0.5f * (rot_end + rot_base) + other;
//    auto from = tf * glm::vec4(glm::rotate(rot_base - center,
//                                           glm::half_pi<float>(), other) +
//                               opposite_center,
//                               1.0f);
//    auto to = tf * glm::vec4(glm::rotate(rot_end - center,
//                                         glm::half_pi<float>(), other) +
//                             opposite_center,
//                             1.0f);
//
//    screen_direction = glm::normalize(from - to);
//}
//
//SliceRotator::~SliceRotator() = default;
//
//void SliceRotator::onDrag(const glm::vec2& delta) {
//    Slice* slice = comp_.draggedSlice();
//    const auto& o = slice->orientation4();
//
//    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
//    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
//    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);
//
//    auto a = base - rot_base;
//    auto b = base + axis1 - rot_base;
//    auto c = base + axis2 - rot_base;
//
//    auto weight = glm::dot(delta, screen_direction);
//    a = glm::rotate(a, weight, rot_end - rot_base) + rot_base;
//    b = glm::rotate(b, weight, rot_end - rot_base) + rot_base;
//    c = glm::rotate(c, weight, rot_end - rot_base) + rot_base;
//
//    slice->setOrientation(a, b - a, c - a);
//}

} // namespace recastx::gui
