#include <memory>
#include <iostream>

#include <imgui.h>

#include "tomcat/tomcat.hpp"

#include "scenes/scene2d.hpp"
#include "scenes/overview_scene.hpp"
#include "window.hpp"


namespace tomcat::gui {

MainWindow::MainWindow() {
    std::unique_ptr<Scene> scene = std::make_unique<OverviewScene>();
    addScene("TOMCAT live 3D preview", std::move(scene));
};

MainWindow::~MainWindow() = default;

void MainWindow::addScene(const std::string& name, std::unique_ptr<Scene>&& scene) {
    scene->addPublisher(this);
    scenes_[name] = std::move(scene);
    if (active_scene_ == nullptr) activate(name);
}

void MainWindow::describe() {
    // Width > 100, Height > 100
    ImGui::SetNextWindowSizeConstraints(ImVec2(280, 500), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Scene controls");
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    active_scene_->describe();

    ImGui::End();
}

void MainWindow::activate(const std::string& name) {
    if (scenes_.find(name) == scenes_.end())
        throw std::runtime_error("Scene " + name + " does not exist!");
    active_scene_ = scenes_[name].get();
}

Scene& MainWindow::scene() { return *active_scene_; }

void MainWindow::render(glm::mat4 window_matrix) {
    active_scene_->draw(window_matrix);
}

void MainWindow::tick(float dt) {
    active_scene_->tick(dt);
}

bool MainWindow::handleMouseButton(int button, int action) {
    return active_scene_->handleMouseButton(button, action);
}

bool MainWindow::handleScroll(double offset) {
    return active_scene_->handleScroll(offset);
}

bool MainWindow::handleMouseMoved(double x, double y) {
    return active_scene_->handleMouseMoved(x, y);
}

bool MainWindow::handleKey(int key, bool down, int mods) {
    return active_scene_->handleKey(key, down, mods);
}

} // tomcat::gui
