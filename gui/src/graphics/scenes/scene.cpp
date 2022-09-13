#include <spdlog/spdlog.h>

#include <imgui.h>

#include "graphics/scenes/scene.hpp"
#include "graphics/scenes/scene_camera.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/scenes/recon_component.hpp"
#include "client.hpp"

namespace tomcat::gui {

Scene::Scene() = default;

Scene::~Scene() {
    glDeleteVertexArrays(1, &vao_handle_);
    glDeleteBuffers(1, &vbo_handle_);
}

void Scene::addComponent(std::unique_ptr<SceneComponent> component) {
    components_.insert(std::make_pair(component->identifier(), std::move(component)));
}

void Scene::describe() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(280, 500), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Image tool (3D)");
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    camera_->describe();
}

bool Scene::handleMouseButton(int button, int action) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleMouseButton(button, action)) return true;
    }

    if (camera_ && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(double offset) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleScroll(offset)) return true;
    }

    if (camera_ && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(double x, double y) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleMouseMoved(x, y)) return true;
    }

    if (camera_ && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, int action, int mods) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleKey(key, action, mods)) return true;
    }

    if (camera_ && camera_->handleKey(key, action, mods)) return true;
    return false;
}

SceneCamera& Scene::camera() { return *camera_; }

void Scene::tick(double time_elapsed) {
    camera_->tick(time_elapsed);
    for (auto& id_and_comp : components_) {
        id_and_comp.second->tick(time_elapsed);
    }

    auto& packets = client_->packets();
    while (!packets.empty()) {
        auto data = std::move(packets.front());
        packets.pop();

        switch (data.first) {
            case PacketDesc::slice_data: {
                auto packet = dynamic_cast<SliceDataPacket*>(data.second.get());
                dynamic_cast<ReconComponent*>(components_["reconstruction"].get())->setSliceData(
                        std::move(packet->data), packet->slice_size, packet->slice_id);
                spdlog::info("Set slice data {}", packet->slice_id);
                break;
            }
            case PacketDesc::volume_data: {
                auto packet = dynamic_cast<VolumeDataPacket*>(data.second.get());
                dynamic_cast<ReconComponent*>(components_["reconstruction"].get())->setVolumeData(
                        std::move(packet->data), packet->volume_size);
                spdlog::info("Set volume data");
                break;
            }
            default: {
                spdlog::warn("Unknown package descriptor: 0x{0:x}",
                             std::underlying_type<PacketDesc>::type(data.first));
                break;
            }
        }

    }
}

void Scene::setPublisher(Client *client) { client_ = client; }

}  // tomcat::gui
