#pragma once

#include <string>
#include <vector>

#include "graphics/interface/window.hpp"
#include "input_handler.hpp"

namespace gui {

class SceneList;

class SceneSwitcher : public Window, public InputHandler {
  public:
    explicit SceneSwitcher(SceneList& scenes);
    ~SceneSwitcher() override;

    void describe() override;

    bool handle_key(int key,  bool down, int mods) override;
    [[nodiscard]] int priority() const override { return 2; }

    void next_scene();
    void add_scene_3d();

    void delete_scene();

  private:
    SceneList& scenes_;

    std::vector<std::string> short_options_;
    std::vector<std::string> model_options_;
};

} // namespace gui
