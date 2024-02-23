/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
#include <mutex>
#include <string>
#include <tuple>

#include "graphics/items/graphics_item.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/light.hpp"
#include "graphics/material.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"
#include "graphics/textures.hpp"
#include "utils.hpp"

namespace recastx::gui {

class Slice;
class Volume;
class Wireframe;


class LightComponent {

    Light light_;

    bool auto_pos_;
    float pos_[3];

    glm::vec3 color_;
    float ambient_;
    float diffuse_;
    float specular_;

  public:

    LightComponent();

    void renderIm();

    void updatePos(const glm::vec3& pos);

    [[nodiscard]] const Light& light() const { return light_; }
};

class MaterialComponent {

    Volume* volume_;

    Material material_;

    float color_[3];

public:

    explicit MaterialComponent(Volume* volume);

    void renderIm();

    [[nodiscard]] const Material& material() const { return material_; }
};

class RenderComponent {

    Volume* volume_;

  public:

    explicit RenderComponent(Volume* volume);

    void renderIm();
};

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

    std::mutex slice_mtx_;
    std::vector<std::tuple<uint64_t, int, std::unique_ptr<Slice>>> slices_;

    GLuint rotation_axis_vao_;
    GLuint rotation_axis_vbo_;

    std::unique_ptr<DragMachine> drag_machine_;
    Slice* dragged_slice_ = nullptr;
    Slice* hovered_slice_ = nullptr;

    Colormap cm_;
    bool auto_levels_ = true;
    bool clamp_negatives_ = true;
    bool update_min_max_val_ = false;
    float min_val_;
    float max_val_;

    std::mutex volume_mtx_;
    std::unique_ptr<Volume> volume_;
    int volume_policy_ = PREVIEW_VOL;
    float volume_front_ = 0.f;
    float volume_front_step_ = 0.01f;
    static constexpr float volume_front_min_ = 0.0f;
    static constexpr float volume_front_max_ = 1.0f;

    LightComponent light_comp_;
    MaterialComponent material_comp_;
    RenderComponent render_comp_;

    std::unique_ptr<Wireframe> wireframe_;
    bool show_wireframe_ = true;

    glm::mat4 matrix_;

    bool show_statistics_ = true;
    ImVec2 st_win_pos_;
    ImVec2 st_win_size_;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;

    FpsCounter slice_counter_;
    FpsCounter volume_counter_;

    template<size_t index>
    void renderImSliceControl(const char* header);

    void renderImVolumeControl();

    bool updateServerSliceParams();

    bool updateServerVolumeParams();

    void updateHoveringSlice(float x, float y);

    std::vector<Slice*> sortedSlices() const;

    void maybeSwitchDragMachine(DragType type);

    void updateMinMaxValues();

public:

    explicit ReconItem(Scene& scene);

    ~ReconItem() override;

    void renderIm() override;

    void onWindowSizeChanged(int width, int height) override;

    void preRenderGl() override;

    void renderGl() override;

    void onFramebufferSizeChanged(int width, int height) override;

    bool updateServerParams() override;

    bool setSliceData(const rpc::ReconSlice& data);

    bool setVolumeData(const rpc::ReconVolumeShard& shard);

    bool consume(const DataType& packet) override;

    bool handleMouseButton(int button, int action) override;

    bool handleMouseMoved(float x, float y) override;

    Slice* draggedSlice() { return dragged_slice_; }
    void setDraggedSlice(Slice* slice) { dragged_slice_ = slice; }

    [[nodiscard]] Slice* hoveredSlice() { return hovered_slice_; }

    [[nodiscard]] bool histogramVisible() const { return show_statistics_; }
    void setHistogramVisible(bool visible) { show_statistics_ = visible; }

    void moveVolumeFrontForward();
    void moveVolumeFrontBackward();
};

} // namespace recastx::gui

#endif // GUI_RECONITEM_H