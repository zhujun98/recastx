/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/volume_component.hpp"
#include "graphics/slice_component.hpp"
#include "graphics/marcher.hpp"
#include "graphics/voxel_object.hpp"
#include "graphics/mesh_object.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/widgets/widget.hpp"
#include "models/cube_model.hpp"

namespace recastx::gui {

VolumeComponent::VolumeComponent(RpcClient* client)
        : Component(client),
          marcher_(new Marcher),
          render_quality_(static_cast<int>(RenderQuality::MEDIUM)) {
    CubeModel data(256);
    data.genSpheres(40);
    setData(data.data(), data.x(), data.x(), data.x());
};

VolumeComponent::~VolumeComponent() = default;

void VolumeComponent::setMeshObject(MeshObject* obj) {
    mesh_object_ = obj;
    mesh_object_->setVisible(false);
}

void VolumeComponent::setVoxelObject(VoxelObject* obj) {
    voxel_object_ = obj;
    voxel_object_->setVisible(true);
}

void VolumeComponent::draw(rpc::ServerState_State) {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    bool expand = ImGui::CollapsingHeader("Volume##VOLUME_COMP", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();

    if (expand) {
        bool cd = false;
        int prev_policy = display_policy_;
        cd |= ImGui::RadioButton("Preview##VOLUME_COMP", &display_policy_, PREVIEW);
        ImGui::SameLine();
        cd |= ImGui::RadioButton("Show##VOLUME_COMP", &display_policy_, SHOW);
        ImGui::SameLine();
        cd |= ImGui::RadioButton("Disable##VOLUME_COMP", &display_policy_, DISABLE);

        if (cd) {
            client_->setVolume(display_policy_ != DISABLE);

            // remove the leftover
            if (prev_policy == DISABLE && display_policy_ != DISABLE) {
                std::lock_guard lck(mtx_);
                buffer_.clear();
            }

            slice_comp_->setSampleVolume(display_policy_ != DISABLE);

            if (display_policy_ == SHOW) slice_comp_->onShowVolume();
        }

        if (display_policy_ == SHOW) {
            ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "RENDERING");

            if (ImGui::SliderInt("Quality##VOLUME_COMP", &render_quality_,
                                 static_cast<int>(RenderQuality::VERY_LOW),
                                 static_cast<int>(RenderQuality::VERY_HIGH))) {
                setRenderQuality(RenderQuality(render_quality_));
            }

            ImGui::RadioButton("Voxel##VOLUME_COMP", &render_policy_, VOXEL);
            ImGui::SameLine();
            ImGui::RadioButton("ISO Surface##VOLUME_COMP", &render_policy_, SURFACE);

            if (render_policy_ == VOXEL) {
                voxel_object_->setVisible(true);
                mesh_object_->setVisible(false);
            }
            else if (render_policy_ == SURFACE) {
                voxel_object_->setVisible(false);
                mesh_object_->setVisible(true);

                if (ImGui::SliderFloat("ISO Value##VOLUME_COMP", &iso_value_, 0.f, 1.f, "%.3f",
                                       ImGuiSliderFlags_AlwaysClamp)) {
                    marcher_->setIsoValue(iso_value_);
                }
            }

            if (voxel_object_->visible()) {
                voxel_object_->renderGUI();
                MaterialManager::instance().getWidget(voxel_object_->materialID())->draw();
            }
            if (mesh_object_->visible()) {
                mesh_object_->renderGUI();
                MaterialManager::instance().getWidget(mesh_object_->materialID())->draw();
            }
        } else {
            voxel_object_->setVisible(false);
            mesh_object_->setVisible(false);
        }
    }
}

bool VolumeComponent::drawStatistics(rpc::ServerState_State) {
    if (display_policy_ == SHOW) {
        std::lock_guard lk(mtx_);
        if (ImPlot::BeginPlot("Volume##Histogram", ImVec2(-1.f, -1.f))) {
            ImPlot::SetupAxes("Pixel value", "Density", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            if (!data_.empty()) {
                const auto& [bin_centers, bin_counts] = data_.histogram();
                float bin_width = 1.f;
                if (bin_centers.size() > 1) bin_width = bin_centers[1] - bin_centers[0];

                ImPlot::PlotBars("##Histogram_Volume",
                                 bin_centers.data(), bin_counts.data(), bin_centers.size(), 0.5f * bin_width);
            }
            ImPlot::EndPlot();
        }
        return true;
    }
    return false;
}

void VolumeComponent::preRender() {
    if (display_policy_ != DISABLE) {
        std::lock_guard lk(mtx_);

        auto mat = MaterialManager::instance().getMaterial<TransferFunc>(voxel_object_->materialID());
        const auto& v = data_.minMaxVals();
        if (v) mat->registerMinMaxVals(v.value());
        
        if (update_texture_) {
            if (data_.empty()) {
                voxel_object_->resetIntensity();
            } else {
                voxel_object_->setIntensity(data_.data(), data_.x(), data_.y(), data_.z());
            }
            update_texture_ = false;
        }
    }

    if (mesh_object_->visible() && (update_mesh_ || marcher_->configChanged())) {
        if (data_.empty()) {
            // TODO
        } else {
            auto [v_min, v_max] = data_.minMaxVals().value();
            mesh_object_->setVertices(marcher_->march(data_, v_min, v_max));
        }
        update_mesh_ = false;
    }
}

RpcClient::State VolumeComponent::updateServerParams() const {
    CHECK_CLIENT_STATE(client_->setVolume(display_policy_ != DISABLE))
    return RpcClient::State::OK;
}

bool VolumeComponent::setShard(uint32_t pos, const std::string& data, uint32_t x, uint32_t y, uint32_t z) {
    if (pos == 0 && buffer_.resize(x, y, z)) {
        spdlog::warn("Volume data shape changed to {} x {} x {}", x, y, z);
    }

    bool ready = buffer_.setShard(data, pos);
    if (ready) {
        {
            std::lock_guard lck(mtx_);
            buffer_.swap(data_);
            if (voxel_object_->visible()) data_.histogram();
            update_texture_ = true;
            update_mesh_ = true;
        }

        spdlog::info("New volume data is ready: {} x {} x {}", x, y, z);
    } else {
        spdlog::debug("Set volume shard: {} x {} x {} ({:.1f}%)",
                      x, y, z, 100.f * float(pos) / float(x * y * z));
    }
    return ready;
}

void VolumeComponent::setRenderQuality(RenderQuality quality) {
    voxel_object_->setRenderQuality(quality);
    if (quality == RenderQuality::VERY_LOW) {
        marcher_->setGridSize(16, 16, 16);
    } else if (quality == RenderQuality::LOW) {
        marcher_->setGridSize(8, 8, 8);
    } else if (quality == RenderQuality::MEDIUM) {
        marcher_->setGridSize(4, 4, 4);
    } else if (quality == RenderQuality::HIGH) {
        marcher_->setGridSize(2, 2, 2);
    } else { // (quality == RenderQuality::VERY_HIGH) {
        marcher_->setGridSize(1, 1, 1);
    }
}

void VolumeComponent::onShowSlice() {
    if (display_policy_ == SHOW) display_policy_ = PREVIEW;
}

void VolumeComponent::setData(const std::string& data, uint32_t x, uint32_t y, uint32_t z) {
    data_.setData(data, x, y, z);
    update_texture_ = true;
    update_mesh_ = true;
}

} // namespace recastx::gui