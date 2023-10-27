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
#include "graphics/aesthetics.hpp"
#include "graphics/image_buffer.hpp"
#include "logger.hpp"

namespace recastx::gui {

ProjectionItem::ProjectionItem(Scene& scene)
        : GraphicsItem(scene),
          buffer_(new ImageBuffer),
          cm_(new Colormap) {
    scene.addItem(this);

    buffer_->keepAspectRatio(true);
    buffer_->clear();

    cm_->set(ImPlotColormap_Greys);
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

    // Caveat: don't use framebuffer size because of high-resolution display like Retinal
    static constexpr int K_PADDING = 5;
    img_size_ = { Style::PROJECTION_WIDTH * (float)width - 2 * K_PADDING,
                  Style::PROJECTION_HEIGHT * (float)height - 2 * K_PADDING - 2 * Style::LINE_HEIGHT };

    buffer_->resize(static_cast<int>(img_size_.x), static_cast<int>(img_size_.y));
}

void ProjectionItem::renderIm() {
    bool cd = ImGui::Checkbox("Show projection: ", &visible_);
    if (visible_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);
        static constexpr int K_PADDING = 5;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(K_PADDING, K_PADDING));
        ImGui::Begin("Raw projection", NULL, ImGuiWindowFlags_NoDecoration);

        ImGui::Image((void*)(intptr_t)buffer_->texture(), img_size_);

        ImGui::Checkbox("Auto Levels", &auto_levels_);

        float step_size = (max_val_ - min_val_) / 100.f;
        if (step_size < 0.01f) step_size = 0.01f; // avoid a tiny step size
        ImGui::DragFloatRange2("Min / Max", &min_val_, &max_val_, step_size,
                               std::numeric_limits<float>::lowest(), // min() does not work
                               std::numeric_limits<float>::max());

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
        buffer_->clear();
        setProjectionId();
    }
    ImGui::PopItemWidth();

    ImGui::Separator();
}

void ProjectionItem::onFramebufferSizeChanged(int /*width*/, int /*height*/) {}

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

void ProjectionItem::updateProjection(uint32_t id, const std::string& data, const std::array<uint32_t, 2>& size) {
    id_ = static_cast<int>(id);

    // TODO: check whether id == id_. This can only be implemented when the GUI client knows the maximum id

    ImageDataType img(size[0] * size[1]);
    std::memcpy(img.data(), data.data(), data.size());
    assert(data.size() == img.size() * sizeof(ImageDataType::value_type));

    auto w = static_cast<int>(size[0]);
    auto h = static_cast<int>(size[1]);
    texture_.setData(img, w, h);

    cm_->bind();
    texture_.bind();
    if (auto_levels_) {
        auto [min_p, max_p] = std::minmax_element(img.begin(), img.end());
        min_val_ = static_cast<float>(*min_p);
        max_val_ = static_cast<float>(*max_p);
    }
    buffer_->render(w, h, min_val_, max_val_);
    texture_.unbind();
    cm_->unbind();

    scene_.useViewport();
}

} // namespace recastx::gui
