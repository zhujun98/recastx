#ifndef GUI_SCENES_SCENE2D_H
#define GUI_SCENES_SCENE2D_H

#include "scene.hpp"


namespace tomop::gui {

class Scene2d : public Scene {

    GLuint texture_id_;
    std::vector<int> size_;

  public:

    Scene2d();
    ~Scene2d() override;

    void draw(glm::mat4 window_matrix) override;
};

} // tomop::gui

#endif // GUI_SCENES_SCENE2D_H