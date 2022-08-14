#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "scenes/scene.hpp"
#include "shaders/shader_program.hpp"
#include "object_component.hpp"
#include "slice.hpp"
#include "textures.hpp"
#include "util.hpp"

namespace tomcat::gui {

class ReconComponent : public ObjectComponent {

    enum class DragType : int { none, rotator, translator};

    class DragMachine {

      protected:

        ReconComponent& comp_;
        glm::vec2 initial_;

        DragType type_;

      public:

        DragMachine(ReconComponent& comp, const glm::vec2& initial, DragType type);
        virtual ~DragMachine();

        virtual void onDrag(glm::vec2 delta) = 0;

        [[nodiscard]] constexpr DragType type() const { return type_; }
    };

    class SliceTranslator : public DragMachine {

      public:

        SliceTranslator(ReconComponent& comp, const glm::vec2& initial);
        ~SliceTranslator() override;

        void onDrag(glm::vec2 delta) override;
    };

    class SliceRotator : public DragMachine {

      public:

        SliceRotator(ReconComponent& comp, const glm::vec2& initial);
        ~SliceRotator() override;

        void onDrag(glm::vec2 delta) override;

        glm::vec3 rot_base;
        glm::vec3 rot_end;
        glm::vec2 screen_direction;
    };

    std::map<int, std::unique_ptr<Slice>> slices_;

    std::vector<float> volume_data_;
    Texture3d<float> volume_texture_;
    GLuint cm_texture_id_;

    glm::mat4 volume_transform_;

    GLuint vao_handle_;
    GLuint vbo_handle_;
    std::unique_ptr<ShaderProgram> program_;

    GLuint line_vao_handle_;
    GLuint line_vbo_handle_;

    GLuint cube_vao_handle_;
    GLuint cube_vbo_handle_;
    GLuint cube_index_handle_;
    int cube_index_count_;
    std::unique_ptr<ShaderProgram> cube_program_;

    Scene& scene_;
    int next_idx_ = 3;

    std::unique_ptr<DragMachine> drag_machine_;
    Slice* dragged_slice_ = nullptr;
    Slice* hovered_slice_ = nullptr;

    std::vector<float> histogram_;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;

    float lower_value_ = -1.0f;
    float upper_value_ = 1.0f;
    bool value_not_set_ = true;
    float volume_min_ = 0.0f;
    float volume_max_ = 1.0f;

    bool show_ = true;

    void initSlices();
  
    void initVolume();
  
    void updateSliceImage(Slice* slice);

    void updateHoveringSlice(float x, float y);

    void maybeSwitchDragMachine(DragType type);

public:

    explicit ReconComponent(Scene& scene);
    ~ReconComponent() override;

    void draw(const glm::mat4& world_to_screen) override;
    void describe() override;

    void setSliceData(std::vector<float>& data,
                      const std::array<int32_t, 2>& size,
                      int slice_idx);

    void setVolumeData(std::vector<float>& data,
                       const std::array<int32_t, 3>& volume_size);

    void update_histogram(const std::vector<float>& data);

    void requestSlices();

    bool handleMouseButton(int button, int action) override;

    bool handleMouseMoved(double x, double y) override;

    glm::mat4 volume_transform() { return volume_transform_; }
    auto& scene() { return scene_; }
    auto& dragged_slice() { return dragged_slice_; }
    auto hovered_slice() { return hovered_slice_; }
    auto& slices() { return slices_; }

    [[nodiscard]] std::string identifier() const override { return "reconstruction"; }

    std::pair<float, float> overall_min_and_max();

    auto generate_slice_idx() { return next_idx_++; }

    static std::tuple<bool, float, glm::vec3> intersectionPoint(
        glm::mat4 inv_matrix, glm::mat4 orientation, glm::vec2 point);
};

} // tomcat::gui
