/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/projection_component.hpp"
#include "graphics/image_object.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/widgets/widget.hpp"

namespace recastx::gui {

ProjectionComponent::ProjectionComponent(RpcClient* client)
        : Component(client) {
    client_->toggleProjectionStream(display_);
}

ProjectionComponent::~ProjectionComponent() = default;

void ProjectionComponent::draw(rpc::ServerState_State) {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    if (ImGui::CollapsingHeader("PROJECTION", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("Display##PROJ_COMP", &display_)) {
            client_->toggleProjectionStream(display_);
            image_object_->setVisible(display_);
        }

        if (display_) {
            ImGui::SameLine();
            ImGui::PushItemWidth(120);
            int prev_id = id_;
            ImGui::InputInt("##PROJ_COMP_ID", &id_);
            id_ = std::clamp(id_, 0, K_MAX_ID_);
            // It is not necessary to immediately clear the displayed image since
            // there is an indicator if the displayed id is not the requested id.
            if (prev_id != id_) client_->setProjection(id_);
            if (displayed_id_ != id_) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                ImGui::Text("%i", displayed_id_);
                ImGui::PopStyleColor();
            }
            ImGui::PopItemWidth();

            MaterialManager::instance().getWidget(image_object_->materialID())->draw();

            image_object_->renderGUI();
        }
    }
    ImGui::PopStyleColor();
}

RpcClient::State ProjectionComponent::updateServerParams() const {
    CHECK_CLIENT_STATE(client_->setProjection(id_))
    return RpcClient::State::OK;
}

void ProjectionComponent::setData(uint32_t id, const std::string& data, uint32_t x, uint32_t y) {
    displayed_id_ = static_cast<int>(id);

    {
        std::lock_guard lck(mtx_);
        data_.setData(data, x, y);
        update_texture_ = true;
    }

    log::info("Set projection data: {} ({} x {})", id, x, y);
}

void ProjectionComponent::preRender() {
    if (display_) {
        std::lock_guard lck(mtx_);

        if (update_texture_) {
            if (data_.empty()) {
                image_object_->resetIntensity();
            } else {
                image_object_->setIntensity(data_.data(), data_.x(), data_.y());
            }
            update_texture_ = false;
        }

        auto mat = MaterialManager::instance().getMaterial<TransferFunc>(image_object_->materialID());
        const auto& v = data_.minMaxVals();
        if (v) mat->registerMinMaxVals(v.value());
    }
}

} // namespace recastx::gui