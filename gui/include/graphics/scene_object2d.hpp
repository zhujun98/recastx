#ifndef GUI_GRAPHICS_SCENE_OBJECT2D_H
#define GUI_GRAPHICS_SCENE_OBJECT2D_H

#include "scene_object.hpp"


namespace gui {

class SceneObject2d : public SceneObject {

    GLuint texture_id_;
    std::vector<int> size_;

  public:

    SceneObject2d();
    ~SceneObject2d() override;

    void draw(glm::mat4 window_matrix) override;
};

} // namespace gui

#endif // GUI_GRAPHICS_SCENE_OBJECT2D_H