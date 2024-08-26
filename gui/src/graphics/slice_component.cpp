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
        if (!slice.object->visible()) continue;

        auto mat = MaterialManager::instance().getMaterial<TransferFunc>(slice.object->materialID());
        const auto &v = slice.data.minMaxVals();
        if (v) mat->registerMinMaxVals(v.value());

        if (slice.update_texture) {
            if (slice.data.empty()) {
                slice.object->resetIntensity();
            } else {
                slice.object->setIntensity(slice.data.data(), slice.data.x(), slice.data.y());
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

void SliceComponent::drawStatistics(rpc::ServerState_State) const {
    ImPlot::BeginSubplots("##Histogram_SLICES", 1, MAX_NUM_SLICES, ImVec2(-1.f, -1.f));
    std::lock_guard lk(mtx_);
    for (auto& slice: slices_) {
        std::string name = "Slice-" + std::to_string(slice.id);
        if (ImPlot::BeginPlot(name.c_str(), ImVec2(-1.f, -1.f))) {
            ImPlot::SetupAxes("Pixel value", "Density", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            if (slice.object->visible()) {
                ImPlot::PlotHistogram(("##Histogram_" + name).c_str(),
                                      slice.data.data(),
                                      static_cast<int>(slice.data.size()),
                                      120,
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

    if (slice.object->isDragging()) return false;

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
    static const char* OFFSET[] = {"Offset##SLICE_COMP_0", "Offset##SLICE_COMP_1", "Offset##SLICE_COMP_2"};

    auto& slice = slices_[index];
    auto object = slice.object;

    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    bool expand = ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen);
    bool highlighting = ImGui::IsItemHovered();

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
                    client_->setSlice(slice.timestamp, object->orientation());
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
            ImGui::BeginDisabled(slice.display_policy == DISABLE);

            // synchronize
            if (slice.dragging && !object->isDragging()) {
                slice.timestamp += MAX_NUM_SLICES;
                client_->setSlice(slice.timestamp, slice.object->orientation());
            }
            slice.dragging = object->isDragging();
            slice.offset = slice.object->offset();

            auto ret = ImGui::SliderFloat(OFFSET[index], &slice.offset,
                                          SliceObject::MIN_OFFSET, SliceObject::MAX_OFFSET, "%.3f",
                                          ImGuiSliderFlags_AlwaysClamp);
            highlighting |= ImGui::IsItemHovered();

            if (ret) {
                object->setOffset(slice.offset);
                object->setHighlighting(true);
            }

            if (ImGui::IsItemDeactivatedAfterEdit()) {
                slice.timestamp += MAX_NUM_SLICES;
                client_->setSlice(slice.timestamp, object->orientation());
                object->setHighlighting(false);
            }
            ImGui::EndDisabled();
        }
    }

    object->setHighlighting(highlighting);
}

void SliceComponent::reset(Slice& slice) {
    slice.offset = 0;
    slice.object->setOffset(0);
    if (slice.plane == Plane::YZ) {
        slice.object->setOrientation(glm::vec3( 0.0f, -0.5f, -0.5f),
                                     glm::vec3( 0.0f,  1.0f,  0.0f),
                                     glm::vec3( 0.0f,  0.0f,  1.0f));
    } else if (slice.plane == Plane::XZ) {
        slice.object->setOrientation(glm::vec3(-0.5f,  0.0f, -0.5f),
                                     glm::vec3( 0.0f,  0.0f,  1.0f),
                                     glm::vec3( 1.0f,  0.0f,  0.0f));
    } else if (slice.plane == Plane::XY) {
        slice.object->setOrientation(glm::vec3(-0.5f, -0.5f,  0.0f),
                                     glm::vec3( 1.0f,  0.0f,  0.0f),
                                     glm::vec3( 0.0f,  1.0f,  0.0f));
    }
}

} // namespace recastx::gui
