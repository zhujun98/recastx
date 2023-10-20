/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/projection_item.hpp"
#include "graphics/projection.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"
#include "logger.hpp"

namespace recastx::gui {

ProjectionItem::ProjectionItem(Scene& scene) : GraphicsItem(scene){
    scene.addItem(this);
}

ProjectionItem::~ProjectionItem() = default;

void ProjectionItem::onWindowSizeChanged(int width, int height) {
    size_ = {
            Style::PROJECTION_WIDTH * (float)width,
            Style::PROJECTION_HEIGHT * (float)height
    };

    pos_ = {
            (1.0f - Style::MARGIN - Style::PROJECTION_WIDTH) * (float)width,
            (1.0f - Style::PROJECTION_HEIGHT - Style::STATUS_BAR_HEIGHT - 2.f * Style::MARGIN) * (float)(height)
    };
}

void ProjectionItem::renderIm() {
    bool cd = ImGui::Checkbox("Show projection: ", &visible_);
    if (visible_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(K_PADDING_, K_PADDING_));
        ImGui::Begin("Raw projection", NULL, ImGuiWindowFlags_NoDecoration);
        if (proj_ != nullptr) {
            ImGui::Image((void*)(intptr_t)proj_->texture(), img_size_);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    if (cd) toggleProjectionStream();

    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    int prev_id = id_;
    ImGui::InputInt("##PROJECTION_ID", &id_);
    id_ = std::clamp(id_, 0, K_MAX_ID_);
    if (prev_id != id_) {
        proj_.reset();
        setProjectionId();
    }
    ImGui::PopItemWidth();

    ImGui::Separator();
}

void ProjectionItem::onFramebufferSizeChanged(int width, int height) {
    img_size_ = { Style::PROJECTION_WIDTH * (float)width - 2 * K_PADDING_,
                  Style::PROJECTION_HEIGHT * (float)height - 2 * K_PADDING_ };

    if (proj_ != nullptr) {
        proj_->resize(static_cast<int>(img_size_.x), static_cast<int>(img_size_.y));
    }
}

void ProjectionItem::renderGl() {}

bool ProjectionItem::updateServerParams() {
    toggleProjectionStream();
    return setProjectionId();
}

bool ProjectionItem::consume(const DataType& packet) {
    if (std::holds_alternative<rpc::ProjectionData>(packet)) {
        const auto& data = std::get<rpc::ProjectionData>(packet);

        updateProjection(data.id(), data.data(), {data.col_count(), data.row_count()});
        log::info("Set projection data: {}", data.id());
        return true;
    }

    return false;
}

void ProjectionItem::toggleProjectionStream() {
    scene_.client()->toggleProjectionStream(visible_);
}

bool ProjectionItem::setProjectionId() {
    return scene_.client()->setProjection(id_);
}

void ProjectionItem::updateProjection(uint32_t id, const std::string &data, const std::array<uint32_t, 2> &size) {
    Projection::DataType proj(size[0] * size[1]);
    std::memcpy(proj.data(), data.data(), data.size());
    assert(data.size() == proj.size() * sizeof(Projection::DataType::value_type));

    if (proj_ == nullptr) {
        proj_ = std::make_unique<Projection>(id);
        proj_->resize(static_cast<int>(img_size_.x), static_cast<int>(img_size_.y));
    }
    assert(proj_->id() == id);

    proj_->setData(proj, {size[0], size[1]});
    scene_.useViewport();
}

} // namespace recastx::gui
