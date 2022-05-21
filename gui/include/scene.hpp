#pragma once

#include <iostream>
#include <memory>
#include <map>
#include <vector>

#include <glm/glm.hpp>

#include "input_handler.hpp"
#include "packet_listener.hpp"
#include "ticker.hpp"
#include "graphics/render_target.hpp"
#include "graphics/scene_object.hpp"

namespace gui {

class Scene : public RenderTarget {
  public:
    Scene(const std::string& name, int dimension, int scene_id);
    ~Scene() override;

    void render(glm::mat4 window_matrix) override;

    SceneObject& object() { return *object_; }

    [[nodiscard]] const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    void set_data(std::vector<unsigned char>& data, int slice = 0) {
        object_->set_data(data, slice);
    }

  [[nodiscard]] int dimension() const { return dimension_; }

  private:
    std::unique_ptr<SceneObject> object_;
    std::string name_;
    int dimension_;
    int scene_id_;
};


class SceneList : public RenderTarget,
                  public InputHandler,
                  public PacketPublisher,
                  public PacketListener,
                  public Ticker {
  public:
    SceneList();
    ~SceneList() override;

    int add_scene(const std::string& name,
                  int id = -1,
                  bool make_active = false,
                  int dimension = 2);
    void delete_scene(int index);
    void set_active_scene(int index);
    int reserve_id();

    auto& scenes() { return scenes_; }
    [[nodiscard]] Scene* active_scene() const { return active_scene_; }

    Scene* get_scene(int scene_id) {
      if (scenes_.find(scene_id) == scenes_.end()) {
        std::cout << "Scene " << scene_id << " does not exist";
        return nullptr;
      }
      return scenes_[scene_id].get();
    }

    void tick(float dt) override;

    [[nodiscard]] int active_scene_index() const { return active_scene_index_; }

    void render(glm::mat4 window_matrix) override;

    bool handle_mouse_button(int button, bool down) override;
    bool handle_scroll(double offset) override;
    bool handle_mouse_moved(float x, float y) override;
    bool handle_key(int key, bool down, int mods) override;

    void handle(Packet& packet) override { send(packet); }

  private:
    std::map<int, std::unique_ptr<Scene>> scenes_;
    Scene* active_scene_ = nullptr;
    int active_scene_index_ = -1;
    int give_away_id_ = 0;
};

}  // namespace gui
