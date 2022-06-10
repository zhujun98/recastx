#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "input_handler.hpp"
#include "packet_listener.hpp"
#include "ticker.hpp"
#include "graphics/render_target.hpp"
#include "graphics/scene_object.hpp"

namespace gui {

class Scene : public RenderTarget {

    std::unique_ptr<SceneObject> object_;
    std::string name_;
    int dimension_;

  public:

    Scene(const std::string& name, int dimension);
    ~Scene() override;

    void render(glm::mat4 window_matrix) override;

    SceneObject& object() { return *object_; }

    [[nodiscard]] const std::string& name() const { return name_; }

    void set_data(std::vector<unsigned char>& data, int slice = 0) {
        object_->set_data(data, slice);
    }

    [[nodiscard]] int dimension() const { return dimension_; }
};


class SceneList : public RenderTarget,
                  public InputHandler,
                  public PacketPublisher,
                  public PacketListener,
                  public Ticker {

    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    Scene* active_scene_ = nullptr;

  public:
    SceneList();
    ~SceneList() override;

    void addScene(const std::string& name, int dimension);

    void describe();

    void activate(const std::string& name);

    SceneObject& object();

    void tick(float dt) override;

    void render(glm::mat4 window_matrix) override;

    bool handleMouseButton(int button, bool down) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(float x, float y) override;
    bool handleKey(int key, bool down, int mods) override;

    void handle(tomop::Packet& packet) override { send(packet); }
};

}  // namespace gui
