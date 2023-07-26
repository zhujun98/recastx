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
#include "graphics/scene.hpp"
#include "graphics/style.hpp"
#include "logger.hpp"

namespace recastx::gui {

ProjectionItem::ProjectionItem(Scene& scene)
        : GraphicsItem(scene), img_(0) {
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
    ImGui::Checkbox("Show projection", &visible_);

    if (visible_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);
        ImGui::Begin("Raw projection", NULL, ImGuiWindowFlags_NoDecoration);

        ImGui::Image((void*)(intptr_t)img_.texture(), ImVec2(size_.x, size_.y));

        ImGui::End();
    }
}

bool ProjectionItem::updateServerParams() {
    return false;
}

bool ProjectionItem::consume(const DataType& packet) {
    if (std::holds_alternative<rpc::ProjectionData>(packet)) {
        const auto& data = std::get<rpc::ProjectionData>(packet);

        setProjectionData(data.data(), {data.row_count(), data.col_count()});
        log::info("Set projection data");
        return true;
    }

    return false;
}

void ProjectionItem::setProjectionData(const std::string &data, const std::array<uint32_t, 2> &size) {
    Projection::DataType proj(size[0] * size[1]);
    std::memcpy(proj.data(), data.data(), data.size());
    assert(data.size() == proj.size() * sizeof(Projection::DataType::value_type));
    img_.setData(std::move(proj), {size[0], size[1]});
}

} // namespace recastx::gui
