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
#include "graphics/projection.hpp"

namespace recastx::gui {

class Colormap;
class Framebuffer;
class ShaderProgram;

class ProjectionItem : public GraphicsItem, public GraphicsGLItem, public GraphicsDataItem {

    ImVec2 pos_;
    ImVec2 size_;

    Projection img_;
    ImVec2 img_size_;
    static constexpr size_t img_margin_ = 5;

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;
    std::unique_ptr<Framebuffer> fb_;

    std::unique_ptr<Colormap> cm_;

    bool visible_ = true;

    void setProjectionData(const std::string& data, const std::array<uint32_t, 2>& size);

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