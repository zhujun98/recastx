/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_PROJECTIONITEM_HPP
#define GUI_PROJECTIONITEM_HPP

#include <mutex>
#include <optional>

#include "common/config.hpp"
#include "graphics/textures.hpp"
#include "graphics/viewport.hpp"
#include "graphics/items/graphics_item.hpp"
#include "utils.hpp"

namespace recastx::gui {

class ImageBuffer;
class Colormap;
class FpsCounter;

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

    std::mutex mtx_;
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

    FpsCounter counter_;

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