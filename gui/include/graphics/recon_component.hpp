#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "scene_object.hpp"
#include "shader_program.hpp"
#include "slice.hpp"
#include "textures.hpp"
#include "object_component.hpp"
#include "util.hpp"

namespace gui {

class ReconComponent;

enum class recon_drag_machine_kind : int {
    none,
    rotator,
    translator,
};

class ReconDragMachine {
  protected:
    ReconComponent& comp_;
    glm::vec2 initial_;

  public:
    ReconDragMachine(ReconComponent& comp, glm::vec2 initial)
        : comp_(comp), initial_(initial) {}

    virtual ~ReconDragMachine() = default;

    virtual void on_drag(glm::vec2 delta) = 0;
    virtual recon_drag_machine_kind kind() = 0;
};

class ReconComponent : public ObjectComponent {

    std::map<int, std::unique_ptr<Slice>> slices_;
    std::map<int, std::unique_ptr<Slice>> fixed_slices_;

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

    SceneObject& object_;
    int next_idx_ = 3;

    std::unique_ptr<ReconDragMachine> drag_machine_;
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

    bool dragging_ = false;
    bool hovering_ = false;
    bool show_ = true;
    bool transparency_mode_ = false;

    void initSlices();
  
    void initVolume();
  
    void updateSliceImage(Slice* slice);
  
  public:

    explicit ReconComponent(SceneObject& object);
    ~ReconComponent();

    void draw(glm::mat4 world_to_screen) override;
    void describe() override;

    void setSliceData(std::vector<float>& data,
                      const std::array<int32_t, 2>& size,
                      int slice_idx,
                      bool additive = true);

    void setVolumeData(std::vector<float>& data,
                       const std::array<int32_t, 3>& volume_size);

    void update_histogram(const std::vector<float>& data);

    void requestSlices();

    void switch_if_necessary(recon_drag_machine_kind kind);
    bool handleMouseButton(int button, bool down) override;
    bool handleMouseMoved(double x, double y) override;
    int index_hovering_over(float x, float y);
    void check_hovered(float x, float y);

    glm::mat4 volume_transform() { return volume_transform_; }
    auto& object() { return object_; }
    auto& dragged_slice() { return dragged_slice_; }
    auto hovered_slice() { return hovered_slice_; }
    auto& get_slices() { return slices_; }

    [[nodiscard]] std::string identifier() const override { return "reconstruction"; }

    std::tuple<bool, float, glm::vec3> intersection_point(glm::mat4 inv_matrix,
                                                          glm::mat4 orientation,
                                                          glm::vec2 point);

    std::pair<float, float> overall_min_and_max();

    auto generate_slice_idx() { return next_idx_++; }
};

class SliceTranslator : public ReconDragMachine {
  public:
    using ReconDragMachine::ReconDragMachine;

    void on_drag(glm::vec2 delta) override;
    recon_drag_machine_kind kind() override {
        return recon_drag_machine_kind::translator;
    }
};

class SliceRotator : public ReconDragMachine {
  public:
    SliceRotator(ReconComponent& comp, glm::vec2 initial);

    void on_drag(glm::vec2 delta) override;

    recon_drag_machine_kind kind() override {
        return recon_drag_machine_kind::rotator;
    }

    glm::vec3 rot_base;
    glm::vec3 rot_end;
    glm::vec2 screen_direction;
};

} // namespace gui
