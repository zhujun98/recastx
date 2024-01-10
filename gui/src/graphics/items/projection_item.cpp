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

    buffer_->keepAspectRatio(false);
    buffer_->clear();

    cm_->set(ImPlotColormap_Greys);
}

ProjectionItem::~ProjectionItem() = default;

void ProjectionItem::onWindowSizeChanged(int width, int height) {
    float s = Style::PROJECTION_SIZE * (float)width;
    size_ = { s, s };

    pos_ = {
            (1.0f - Style::MARGIN) * (float)width - s,
            (1.0f - Style::STATUS_BAR_HEIGHT - 2.f * Style::MARGIN) * (float)(height) - s
    };

    // Caveat: don't use framebuffer size because of high-resolution display like Retinal
    static constexpr int K_PADDING = 5;
    img_size_ = { s - 2 * K_PADDING, s - 2 * K_PADDING - 3 * Style::LINE_HEIGHT };
}

void ProjectionItem::renderIm() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "PROJECTION");

    bool cd = ImGui::Checkbox("Show projection: ", &visible_);
    ImGui::SameLine();
    ImGui::PushItemWidth(90);
    int prev_id = id_;
    ImGui::InputInt("##PROJECTION_ID", &id_);
    id_ = std::clamp(id_, 0, K_MAX_ID_);
    if (prev_id != id_) {
        setProjectionId();
    }
    if (displayed_id_ != id_) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        ImGui::Text("%i", displayed_id_);
        ImGui::PopStyleColor();
    }
    ImGui::PopItemWidth();

    if (ImGui::Checkbox("Auto Levels", &auto_levels_) && auto_levels_) {
        update_min_max_vals_ = true;
    }

    bool rerender = false;
    ImGui::BeginDisabled(auto_levels_);
    float step_size = (max_val_ - min_val_) / 100.f;
    if (step_size < 0.01f) step_size = 0.01f; // avoid a tiny step size
    rerender |= ImGui::DragFloatRange2("Min / Max", &min_val_, &max_val_, step_size,
                                       std::numeric_limits<float>::lowest(), // min() does not work
                                       std::numeric_limits<float>::max());
    ImGui::EndDisabled();

    if (visible_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);
        static constexpr int K_PADDING = 5;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(K_PADDING, K_PADDING));
        ImGui::Begin("Raw projection", NULL, ImGuiWindowFlags_NoDecoration);

        rerender |= buffer_->resize(static_cast<int>(img_size_.x), static_cast<int>(img_size_.y));

        ImGui::Image((void*)(intptr_t)buffer_->texture(), img_size_);

        if (rerender && !data_.empty()) renderBuffer(texture_.x(), texture_.y());

        ImGui::End();
        ImGui::PopStyleVar();
    }

    if (cd) toggleProjectionStream();

    ImGui::Separator();

    scene_.setStatus("projectionUpdateFrameRate", counter_.frameRate());
}

void ProjectionItem::onFramebufferSizeChanged(int /*width*/, int /*height*/) {}

void ProjectionItem::preRenderGl() {
    std::lock_guard lck(mtx_);

    if (update_texture_) {
        if (!data_.empty()) {
            texture_.setData(data_, static_cast<int>(shape_[0]), static_cast<int>(shape_[1]));
        }
        renderBuffer(static_cast<int>(shape_[0]), static_cast<int>(shape_[1]));
        update_texture_ = false;
    }

    if (update_min_max_vals_ && auto_levels_) {
        updateMinMaxVals();
        update_min_max_vals_ = false;
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
        counter_.count();
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
    displayed_id_ = static_cast<int>(id);

    ImageDataType img(size[0] * size[1]);
    std::memcpy(img.data(), data.data(), data.size());
    assert(data.size() == img.size() * sizeof(ImageValueType));

    std::lock_guard lck(mtx_);

    data_ = std::move(img);
    shape_ = size;
    update_texture_ = true;
    update_min_max_vals_ = true;
}

void ProjectionItem::updateMinMaxVals() {
    if (data_.empty()) {
        min_val_ = 0.f;
        max_val_ = 0.f;
        return;
    }
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_val_ = static_cast<float>(*vmin);
    max_val_ = static_cast<float>(*vmax);
}

void ProjectionItem::renderBuffer(int width, int height) {
    cm_->bind();
    texture_.bind();
    buffer_->render(width, height, min_val_, max_val_);
    texture_.unbind();
    cm_->unbind();

    scene_.useViewport();
}

} // namespace recastx::gui
