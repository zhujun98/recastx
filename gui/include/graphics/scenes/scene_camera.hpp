#ifndef GUI_SCENE_CAMERA_H
#define GUI_SCENE_CAMERA_H

#include <array>
#include <map>
#include <vector>
#include <utility>
#include <string>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

#include "input_handler.hpp"
#include "ticker.hpp"

namespace tomcat::gui {


class SceneCamera : public InputHandler, public Ticker {

    std::string curr_cm_;
    GLuint cm_texture_id_;

  protected:

    bool interaction_disabled_ = false;

  public:

    using ColormapGradientType = std::array<std::vector<std::pair<double, double>>, 3>;

    [[nodiscard]] static std::map<std::string, ColormapGradientType>& gradients() {
        static std::map<std::string, ColormapGradientType> cms = {
            {"bone", {{
                {{0.0, 0.0}, {0.746032, 0.652778}, {1.0, 1.0}},
                {{0.0, 0.0}, {0.365079, 0.319444}, {0.746032, 0.777778}, {1.0, 1.0}},
                {{0.0, 0.0}, {0.365079, 0.444444}, {1.0, 1.0}}
            }}},
            {"gray", {{
                {{0.0, 0.0}, {1.0, 1.0}},
                {{0.0, 0.0}, {1.0, 1.0}},
                {{0.0, 0.0}, {1.0, 1.0}}
            }}},
            {"hot", {{
                {{0.0, 0.416}, {0.36, 1.0}, {1.0, 1.0}},
                {{0.0, 0.0}, {0.365079, 0.0}, {0.746032, 1.0}, {1.0, 1.0}},
                {{0.0, 0.0}, {0.74, 0.0}, {1.0, 1.0}}
            }}}
        };
        return cms;
    };

    SceneCamera();
    virtual ~SceneCamera();

    virtual glm::mat4 matrix() = 0;

    virtual void lookAt(glm::vec3 /* center */) = 0;

    [[nodiscard]] GLuint colormapTextureId() const { return cm_texture_id_; }
    void setColormap(const std::string& name);

    virtual void describe();

    void toggle_interaction() { interaction_disabled_ = ! interaction_disabled_; }
};

} // namespace tomcat::gui

#endif // GUI_SCENE_CAMERA_H
