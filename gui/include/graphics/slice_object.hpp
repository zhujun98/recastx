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

    GLuint VAO_;
    GLuint VBO_;

    static constexpr float vertices_[] = {
            -0.5f, -0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f
    };

    glm::vec3 normal_;

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
        auto m = rotation_;
        return {
                m[0][0], m[0][1], m[0][2],
                m[1][0], m[1][1], m[1][2],
                m[2][0] + 2.f * pos_[0], m[2][1] + 2.f * pos_[1], m[2][2] + 2.f * pos_[2]
        };
    }

    [[nodiscard]] const glm::vec3& normal() const { return normal_; }

    void setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y);

    void setOffset(float offset);

    void setSampleVolume(bool state) { sample_volume_ = state; }
};

} // recastx::gui

#endif // GUI_SLICE_OBJECT_H
