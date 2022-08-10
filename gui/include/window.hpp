#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "graphics/render_target.hpp"
#include "scenes/scene.hpp"
#include "input_handler.hpp"
#include "packet_publisher.hpp"
#include "ticker.hpp"

namespace gui {

class MainWindow : public RenderTarget,
                   public InputHandler,
                   public PacketPublisher,
                   public Ticker {

    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    Scene* active_scene_ = nullptr;

  public:
    MainWindow();
    ~MainWindow() override;

    void addScene(const std::string& name, int dimension);

    void describe();

    void activate(const std::string& name);

    Scene& scene();

    void tick(float dt) override;

    void render(glm::mat4 window_matrix) override;

    bool handleMouseButton(int button, bool down) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(double x, double y) override;
    bool handleKey(int key, bool down, int mods) override;
};

}  // namespace gui

#endif // GUI_WINDOW_H