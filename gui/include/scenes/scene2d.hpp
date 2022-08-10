#ifndef GUI_GRAPHICS_SCENE2D_H
#define GUI_GRAPHICS_SCENE2D_H

#include "scene.hpp"


namespace gui {

class Scene2d : public Scene {

    GLuint texture_id_;
    std::vector<int> size_;

  public:

    Scene2d();
    ~Scene2d() override;

    void draw(glm::mat4 window_matrix) override;
};

} // namespace gui

#endif // GUI_GRAPHICS_SCENE2D_H