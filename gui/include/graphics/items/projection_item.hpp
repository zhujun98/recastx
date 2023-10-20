/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_PROJECTIONITEM_HPP
#define GUI_PROJECTIONITEM_HPP

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class Projection;

class ProjectionItem : public GraphicsItem, public GraphicsGLItem, public GraphicsDataItem {

    ImVec2 pos_;
    ImVec2 size_;

    std::unique_ptr<Projection> proj_;
    int id_;
    static constexpr int K_MAX_ID_ = 10000;
    static constexpr int K_PADDING_ = 5;
    ImVec2 img_size_;

    bool visible_ = true;

    void toggleProjectionStream();

    bool setProjectionId();

    void updateProjection(uint32_t id, const std::string& data, const std::array<uint32_t, 2>& size);

  public:

    explicit ProjectionItem(Scene& scene);

    ~ProjectionItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;

    bool updateServerParams() override;

    bool consume(const DataType& packet) override;
};

} // namespace recastx::gui

#endif //GUI_PROJECTIONITEM_HPP