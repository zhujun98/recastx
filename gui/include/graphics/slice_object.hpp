/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SLICE_OBJECT_H
#define GUI_SLICE_OBJECT_H

#include <vector>

#include "interactable.hpp"
#include "common/config.hpp"

namespace recastx::gui {

class Renderer;

class SliceObject : public Interactable {

  public:

    static constexpr float MAX_OFFSET =  1.f;
    static constexpr float MIN_OFFSET = -1.f;

  private:

    GLuint VAO_;
    GLuint VBO_;

    static constexpr float vertices_[] = {
            -0.5f, -0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f
    };

    float offset_ { 0.f };
    glm::vec3 normal_;
    glm::mat4 matrix_;

    Texture2D intensity_;
    Texture3D* voxel_intensity_ { nullptr };
    bool sample_volume_ { true };

    void init();

  public:

    SliceObject();

    ~SliceObject() override;

    void render(Renderer* renderer) override;

    template<typename T>
    void setIntensity(const T* data, uint32_t x, uint32_t y) {
        intensity_.setData(data, x, y, 1);
    }

    void resetIntensity() {
        intensity_.invalidate();
    }

    void setVoxelIntensity(Texture3D* intensity) { voxel_intensity_ = intensity; }

    [[nodiscard]] Orientation orientation() const {
        auto m = 2.f * rotation_;
        return {
                m[0][0], m[0][1], m[0][2],
                m[1][0], m[1][1], m[1][2],
                m[2][0] + 2.f * pos_[0], m[2][1] + 2.f * pos_[1], m[2][2] + 2.f * pos_[2]
        };
    }

    [[nodiscard]] std::array<glm::vec3, 3> geometry() const {
        return {glm::vec3(rotation_[0]), glm::vec3(rotation_[1]), glm::vec3(rotation_[2]) + pos_};
    }

    void setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y);

    void setOffset(float offset);

    void translate(float delta);

    [[nodiscard]] float offset() const { return offset_; }

    void setSampleVolume(bool state) { sample_volume_ = state; }

    bool mouseHoverEvent(const MouseHoverEvent& ev) override;

    bool mouseDragEvent(const MouseDragEvent& ev) override;
};

} // recastx::gui

#endif // GUI_SLICE_OBJECT_H
