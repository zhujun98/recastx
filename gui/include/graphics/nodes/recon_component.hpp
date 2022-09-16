#ifndef GUI_RECON_COMPONENT_H
#define GUI_RECON_COMPONENT_H
#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>

#include "scene_component.hpp"
#include "graphics/nodes/scene.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/slice.hpp"
#include "graphics/textures.hpp"
#include "graphics/volume.hpp"

namespace tomcat::gui {

class ReconComponent : public DynamicSceneComponent {

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
    std::unique_ptr<Volume> volume_;

    Colormap cm_;

    glm::mat4 volume_transform_;

    GLuint vao_handle_;
    GLuint vbo_handle_;
    std::unique_ptr<ShaderProgram> solid_shader_;

    GLuint line_vao_handle_;
    GLuint line_vbo_handle_;

    GLuint cube_vao_handle_;
    GLuint cube_vbo_handle_;
    GLuint cube_index_handle_;
    int cube_index_count_;
    std::unique_ptr<ShaderProgram> wireframe_shader_;

    int next_idx_ = 3;

    std::unique_ptr<DragMachine> drag_machine_;
    Slice* dragged_slice_ = nullptr;
    Slice* hovered_slice_ = nullptr;

    bool auto_levels_ = true;
    float min_val_;
    float max_val_;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;

    void initSlices();

    void resetSlices();
  
    void initVolume();

    void updateHoveringSlice(float x, float y);

    void maybeSwitchDragMachine(DragType type);

    void drawSlice(Slice* slice, const glm::mat4& world_to_screen);

    void maybeUpdateMinMaxValues();

public:

    explicit ReconComponent(Scene& scene);

    ~ReconComponent() override;

    void render(const glm::mat4& world_to_screen) override;

    void init() override;

    void describe() override;

    void setSliceData(std::vector<float>&& data,
                      const std::array<uint32_t, 2>& size,
                      int slice_idx);

    void setVolumeData(std::vector<float>&& data,
                       const std::array<uint32_t, 3>& volume_size);

    void consume(const PacketDataEvent& data) override;

    bool handleMouseButton(int button, int action) override;

    bool handleMouseMoved(double x, double y) override;

    glm::mat4 volume_transform() { return volume_transform_; }
    auto& scene() { return scene_; }
    auto& dragged_slice() { return dragged_slice_; }
    auto hovered_slice() { return hovered_slice_; }
    auto& slices() { return slices_; }

    auto generate_slice_idx() { return next_idx_++; }
};

} // namespace tomcat::gui

#endif // GUI_RECON_COMPONENT_H