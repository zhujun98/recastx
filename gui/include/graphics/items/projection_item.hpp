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

#include <optional>

#include "common/config.hpp"
#include "graphics/textures.hpp"
#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class ImageBuffer;
class Colormap;

class ProjectionItem : public GraphicsItem, public GraphicsGLItem, public GraphicsDataItem {

  public:

    using ImageDataType = std::vector<RawDtype>;
    using ImageValueType = typename ImageDataType::value_type;

  private:

    ImVec2 pos_;
    ImVec2 size_;

    int id_ {0};
    static constexpr int K_MAX_ID_ = 10000;
    int displayed_id_ {0};

    ImageDataType data_;
    std::array<uint32_t, 2> shape_;

    ImVec2 img_size_;
    bool update_texture_ = false;
    ImageTexture<ImageValueType> texture_;
    std::unique_ptr<ImageBuffer> buffer_;
    std::unique_ptr<Colormap> cm_;

    bool visible_ { true };
    bool auto_levels_ { true };
    float min_val_ { 0.f };
    float max_val_ { 0.f };
    bool update_min_max_vals_ {false};

    void toggleProjectionStream();

    bool setProjectionId();

    void updateProjection(uint32_t id, const std::string& data, const std::array<uint32_t, 2>& size);

    void updateMinMaxVals();

    void renderBuffer(int width, int height);

  public:

    explicit ProjectionItem(Scene& scene);

    ~ProjectionItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void preRenderGl() override;

    void renderGl() override;

    bool updateServerParams() override;

    bool consume(const DataType& packet) override;
};

} // namespace recastx::gui

#endif //GUI_PROJECTIONITEM_HPP