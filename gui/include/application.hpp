/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include <array>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "graphics/renderer.hpp"
#include "graphics/input_handler.hpp"
#include "graphics/scene.hpp"
#include "rpc_client.hpp"
#include "fps_counter.hpp"

namespace recastx::gui {

class Component;
class ScanComponent;
class LogComponent;
class GeometryComponent;
class PreprocComponent;
class ProjectionComponent;
class VolumeComponent;
class SliceComponent;

class Application {

    int width_;
    int height_;
    const std::string title_ = "RECASTX - REConstruction of Arbitrary Slabs in Tomography X";

    GLFWwindow* glfw_window_ = nullptr;

    struct Layout {
        int w;
        int h;
        std::array<float, 4> left;
        std::array<float, 4> right;
        std::array<float, 4> top;
        std::array<float, 4> bottom;
        std::array<float, 4> status;
        std::array<float, 4> popup;
    };

    Layout layout_;

    std::atomic_bool running_ = false;
    std::thread consumer_thread_;

    rpc::ServerState_State server_state_ = rpc::ServerState_State_UNKNOWN;
    std::unique_ptr<RpcClient> rpc_client_;

    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<InputHandler> input_handler_;

    std::vector<std::unique_ptr<Scene>> scenes_;

    std::unique_ptr<ScanComponent> scan_comp_;
    std::unique_ptr<LogComponent> log_comp_;
    std::unique_ptr<GeometryComponent> geom_comp_;
    std::unique_ptr<PreprocComponent> preproc_comp_;
    std::unique_ptr<ProjectionComponent> proj_comp_;
    std::unique_ptr<VolumeComponent> volume_comp_;
    std::unique_ptr<SliceComponent> slice_comp_;
    std::vector<Component*> components_;

    std::unordered_map<Renderable*, bool> misc_objects_;

    FpsCounter slice_counter_;
    FpsCounter volume_counter_;
    FpsCounter projection_counter_;

    inline static std::unique_ptr<Application> instance_;

    Application();

    void registerCallbacks();

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int);

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    static void scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset);

    static void keyCallback(GLFWwindow* window, int key, int, int action, int mods);

    void onWindowSizeChanged(int width, int height);

    void onFramebufferSizeChanged(int width, int height);

    RpcClient::State updateServerParams();

    void startConsumer();

    bool consume(const RpcClient::DataType& packet);

    void initCenterScene();
    void initTopLeftScene();
    void initTopRightScene();
    void initSatelliteScene();

    void drawLeftGUI();
    void drawRightGUI();
    void drawTopGUI();
    void drawBottomGUI();
    void drawStatusGUI();
    void drawPopupGUI();
    void drawMainControlGUI();

  public:

    ~Application();

    static Application& instance();

    void spin(const std::string& endpoint);

    void connectServer();

    void startAcquiring();

    void stopAcquiring();

    void startProcessing();

    void stopProcessing();
};

}  // namespace recastx::gui

#endif // GUI_APPLICATION_H