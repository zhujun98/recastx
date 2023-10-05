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
#include "graphics/textures.hpp"
#include "utils.hpp"

namespace recastx::gui {

class FpsCounter;
class Slice;
class Volume;
class Wireframe;

class ReconItem : public GraphicsItem, public GraphicsGLItem, public GraphicsDataItem {

    enum class DragType : int { none, rotator, translator};
    
    enum SlicePolicy { SHOW2D_SLI = 0, SHOW3D_SLI = 1, DISABLE_SLI = 2};

    enum VolumePolicy { PREVIEW_VOL = 0, SHOW_VOL = 1, DISABLE_VOL = 2};

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
    std::array<int, MAX_NUM_SLICES> slice_policies_ { SHOW2D_SLI, SHOW2D_SLI, SHOW2D_SLI };

    GLuint rotation_axis_vao_;
    GLuint rotation_axis_vbo_;

    std::unique_ptr<DragMachine> drag_machine_;
    Slice* dragged_slice_ = nullptr;
    Slice* hovered_slice_ = nullptr;

    Colormap cm_;
    bool auto_levels_ = true;
    bool clamp_negatives_ = true;
    float min_val_;
    float max_val_;

    std::unique_ptr<Volume> volume_;
    int volume_policy_ = PREVIEW_VOL;
    float volume_alpha_ = 1.f;

    std::unique_ptr<Wireframe> wireframe_;

    glm::mat4 matrix_;

    bool show_statistics_ = true;
    ImVec2 st_win_pos_;
    ImVec2 st_win_size_;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;

    FpsCounter fps_counter_;

    void initSlices();

    template<size_t index>
    void renderImSliceControl(const char* header);

    void renderImVolumeControl();

    bool updateServerSliceParams();

    bool updateServerVolumeParams();

    void updateHoveringSlice(float x, float y);

    std::vector<Slice*> sortedSlices() const;

    void maybeSwitchDragMachine(DragType type);

    void maybeUpdateMinMaxValues();

public:

    explicit ReconItem(Scene& scene);

    ~ReconItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;

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