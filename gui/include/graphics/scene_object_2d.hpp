#pragma once

#include "scene_object.hpp"


namespace gui {

class SceneObject2d : public SceneObject {

    GLuint texture_id_;
    std::vector<unsigned char> data_;
    std::vector<int> size_;

  protected:

    void update_image_(int slice = 0) override;

  public:

    SceneObject2d();
    ~SceneObject2d() override;

    void draw(glm::mat4 window_matrix) override;

    void set_data(std::vector<unsigned char>& data, int slice = 0) override {
        if (slice != 0) throw;

        data_ = data;
        update_image_();
    }
};

} // namespace gui
