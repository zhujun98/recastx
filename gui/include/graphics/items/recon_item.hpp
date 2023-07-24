/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RECONITEM_H
#define GUI_RECONITEM_H

#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>

#include "graphics/items/graphics_item.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/slice.hpp"
#include "graphics/textures.hpp"
#include "graphics/volume.hpp"
#include "utils.hpp"

namespace recastx::gui {

class FpsCounter;

class ReconItem : public GraphicsItem, public GraphicsGLItem, public GraphicsDataItem {

    enum class DragType : int { none, rotator, translator};

    class DragMachine {

      protected:

        ReconItem& comp_;
        glm::vec2 initial_;

        DragType type_;

      public:

        DragMachine(ReconItem& comp, const glm::vec2& initial, DragType type);
        virtual ~DragMachine();

        virtual void onDrag(const glm::vec2& delta) = 0;

        [[nodiscard]] constexpr DragType type() const { return type_; }
    };

    class SliceTranslator : public DragMachine {

      public:

        SliceTranslator(ReconItem& comp, const glm::vec2& initial);
        ~SliceTranslator() override;

        void onDrag(const glm::vec2& delta) override;
    };

    class SliceRotator : public DragMachine {

      public:

        SliceRotator(ReconItem& comp, const glm::vec2& initial);
        ~SliceRotator() override;

        void onDrag(const glm::vec2& delta) override;

        glm::vec3 rot_base;
        glm::vec3 rot_end;
        glm::vec2 screen_direction;
    };

    friend class SliceRotator;

    std::vector<std::pair<uint64_t, std::unique_ptr<Slice>>> slices_;
    std::unique_ptr<Volume> volume_;

    GLuint slice_vao_;
    GLuint slice_vbo_;
    std::unique_ptr<ShaderProgram> slice_shader_;

    GLuint rotation_axis_vao_;
    GLuint rotation_axis_vbo_;

    GLuint wireframe_vao_;
    GLuint wireframe_vbo_;
    GLuint wireframe_ebo_;
    std::unique_ptr<ShaderProgram> wireframe_shader_;

    std::unique_ptr<DragMachine> drag_machine_;
    Slice* dragged_slice_ = nullptr;
    Slice* hovered_slice_ = nullptr;

    Colormap cm_;
    bool auto_levels_ = true;
    float min_val_;
    float max_val_;

    glm::mat4 matrix_;

    bool show_statistics_ = true;
    ImVec2 st_win_pos_;
    ImVec2 st_win_size_;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;

    FpsCounter fps_counter_;

    void initSlices();

    bool updateServerSliceParams();

    void initVolume();

    void updateHoveringSlice(float x, float y);

    void maybeSwitchDragMachine(DragType type);

    void drawSlice(Slice* slice, const glm::mat4& view, const glm::mat4& projection);

    void maybeUpdateMinMaxValues();

public:

    explicit ReconItem(Scene& scene);

    ~ReconItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void renderGl(const glm::mat4& view,
                  const glm::mat4& projection,
                  const RenderParams& params) override;

    bool updateServerParams() override;

    void setSliceData(const std::string& data,
                      const std::array<uint32_t, 2>& size,
                      uint64_t timestamp);

    void setVolumeData(const std::string& data,
                       const std::array<uint32_t, 3>& volume_size);

    bool consume(const DataType& packet) override;

    bool handleMouseButton(int button, int action) override;

    bool handleMouseMoved(float x, float y) override;

    Slice* draggedSlice() { return dragged_slice_; }
    void setDraggedSlice(Slice* slice) { dragged_slice_ = slice; }

    Slice* hoveredSlice() { return hovered_slice_; }
};

} // namespace recastx::gui

#endif // GUI_RECONITEM_H