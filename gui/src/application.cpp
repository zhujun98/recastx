/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "application.hpp"
#include "graphics/camera.hpp"
#include "graphics/light.hpp"
#include "graphics/voxel_object.hpp"
#include "graphics/mesh_object.hpp"
#include "graphics/slice_object.hpp"
#include "graphics/image_object.hpp"
#include "graphics/simple_object.hpp"
#include "graphics/glyph_object.hpp"
#include "graphics/style.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/log_component.hpp"
#include "graphics/geometry_component.hpp"
#include "graphics/preproc_component.hpp"
#include "graphics/projection_component.hpp"
#include "graphics/scan_component.hpp"
#include "graphics/slice_component.hpp"
#include "graphics/volume_component.hpp"

namespace recastx::gui {

namespace detail {

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

} // detail

Application::Application() : width_(1440), height_(1080) {
    glfwSetErrorCallback(detail::glfwErrorCallback);

    if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    glfw_window_ = glfwCreateWindow(width_, height_, title_.c_str(), NULL, NULL);
    if (glfw_window_ == NULL) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(glfw_window_);
    glfwSetWindowSizeLimits(glfw_window_, Style::MIN_WINDOW_WIDTH, Style::MIN_WINDOW_HEIGHT,
                            GLFW_DONT_CARE, GLFW_DONT_CARE);

    if (gl3wInit()) {
        throw std::runtime_error("Failed to initialize OpenGL!");
    }

    spdlog::info("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    registerCallbacks();

    initRenderer();
}

Application::~Application() {
    running_ = false;

    rpc_client_.reset();
    consumer_thread_.join();

    scenes_.clear();
    renderer_.reset();

    glfwTerminate();
}

Application& Application::instance() {
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Application>(new Application);
    }
    return *instance_;
}

void Application::spin(const std::string& endpoint) {
    rpc_client_ = std::make_unique<RpcClient>(endpoint);

    scan_comp_ = std::make_unique<ScanComponent>(rpc_client_.get());
    components_.push_back(scan_comp_.get());
    log_comp_ = std::make_unique<LogComponent>(rpc_client_.get());
    components_.push_back(log_comp_.get());
    geom_comp_ = std::make_unique<GeometryComponent>(rpc_client_.get());
    components_.push_back(geom_comp_.get());
    preproc_comp_ = std::make_unique<PreprocComponent>(rpc_client_.get());
    components_.push_back(preproc_comp_.get());
    proj_comp_ = std::make_unique<ProjectionComponent>(rpc_client_.get());
    components_.push_back(proj_comp_.get());
    volume_comp_ = std::make_unique<VolumeComponent>(rpc_client_.get());
    components_.push_back(volume_comp_.get());
    slice_comp_ = std::make_unique<SliceComponent>(rpc_client_.get());
    components_.push_back(slice_comp_.get());

    initCenterScene();
    initTopLeftScene();
    initTopRightScene();
    initSatelliteScene();

    glfwGetWindowSize(glfw_window_, &width_, &height_);
    onWindowSizeChanged(width_, height_);

    int w, h;
    glfwGetFramebufferSize(glfw_window_, &w, &h);
    onFramebufferSizeChanged(w, h);

    connectServer();
    rpc_client_->startStreaming();

    startConsumer();

    while (!glfwWindowShouldClose(glfw_window_)) {
        glfwPollEvents();

        for (auto comp : components_) comp->preRender();
        MaterialManager::instance().preRender();

        renderer_->begin();

        for (auto& scene : scenes_) {
            scene->draw(renderer_.get());
        }

        drawLeftGUI();
        drawRightGUI();
        drawTopGUI();
        drawBottomGUI();
        drawStatusGUI();

        renderer_->end();

        glfwSwapBuffers(glfw_window_);
    }
}

void Application::connectServer() {
    auto server_state = rpc_client_->shakeHand();
    if (server_state) {
        auto s = server_state.value();

        log::info("Connected to reconstruction server");
        if (s == rpc::ServerState_State_UNKNOWN) {
            log::error("Reconstruction server in {} state", RpcClient::serverStateToString(s));
        } else {
            log::info("Reconstruction server in {} state", RpcClient::serverStateToString(s));
            if (s != rpc::ServerState_State_READY) {
                log::warn("Client and reconstruction server are not synchronised. "
                          "Manually stop and restart data acquisition and/or processing");
            }
        }

        server_state_ = server_state.value();
    }
}

void Application::startAcquiring() {
    if (updateServerParams() != RpcClient::State::OK) {
        server_state_ = rpc::ServerState_State_UNKNOWN;
        return;
    }

    if (rpc_client_->startAcquiring() == RpcClient::State::OK) {
        server_state_ = rpc::ServerState_State_ACQUIRING;
        log::info("Started acquiring data");
    }
}

void Application::stopAcquiring() {
    if (rpc_client_->stopAcquiring() == RpcClient::State::OK) {
        server_state_ = rpc::ServerState_State_READY;
        log::info("Stopped acquiring data");
    } else {
        server_state_ = rpc::ServerState_State_UNKNOWN;
    }
}

void Application::startProcessing() {
    if (updateServerParams() != RpcClient::State::OK) {
        server_state_ = rpc::ServerState_State_UNKNOWN;
        return;
    }

    if (rpc_client_->startProcessing() == RpcClient::State::OK) {
        server_state_ = rpc::ServerState_State_PROCESSING;
        log::info("Started acquiring & processing data");
    }
}

void Application::stopProcessing() {
    if (rpc_client_->stopProcessing() == RpcClient::State::OK) {
        server_state_ = rpc::ServerState_State_READY;
        log::info("Stopped acquiring & processing data");
    } else {
        server_state_ = rpc::ServerState_State_UNKNOWN;
    }
}

void Application::registerCallbacks() {
    glfwSetWindowSizeCallback(glfw_window_, [](GLFWwindow* /*window*/, int width, int height) {
        instance().onWindowSizeChanged(width, height);
    });

    glfwSetFramebufferSizeCallback(glfw_window_, [](GLFWwindow* /*window*/, int width, int height) {
        instance().onFramebufferSizeChanged(width, height);
    });

    glfwSetMouseButtonCallback(glfw_window_, mouseButtonCallback);
    glfwSetCursorPosCallback(glfw_window_, cursorPosCallback);
    glfwSetScrollCallback(glfw_window_, scrollCallback);
    glfwSetKeyCallback(glfw_window_, keyCallback);
}

void Application::mouseButtonCallback(GLFWwindow*, int button, int action, int) {
//    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, 0);
    auto& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        for (auto& scene : instance().scenes_) {
            scene->handleMouseButton(button, action);
        }
    }
}

void Application::scrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset) {
//    ImGui_ImplGlfw_ScrollCallback(window, 0.0, yoffset);
    auto& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        for (auto& scene : instance().scenes_) {
            scene->handleScroll(static_cast<float>(yoffset));
        }
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
//    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    auto& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        auto [x, y] = Application::normalizeCursorPos(window, xpos, ypos);
        for (auto& scene : instance().scenes_) {
            scene->handleMouseMoved(x, y);
        }
    }
}

void Application::keyCallback(GLFWwindow*, int key, int, int action, int mods) {
//    ImGui_ImplGlfw_KeyCallback(window, key, 0, action, 0);
    auto& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        for (auto& scene : instance().scenes_) {
            scene->handleKey(key, action, mods);
        }
    }
}

void Application::onWindowSizeChanged(int width, int height) {
    layout_.w = width;
    layout_.h = height;

    float mw = std::min((float)Style::MARGIN_MAX, Style::MARGIN * (float) width);
    float mh = std::min((float)Style::MARGIN_MAX, Style::MARGIN * (float) height);
    float lw = Style::LEFT_PANEL_WIDTH * (float) width;
    float rw = Style::RIGHT_PANEL_WIDTH * (float) width;
    float th = Style::TOP_PANEL_HEIGHT * (float) height;
    float bh = Style::BOTTOM_PANEL_HEIGHT * (float) height;

    layout_.left = {mw, mh * 2.f + th, lw, (float)height - 3.f * mh - bh};
    layout_.right = {(float)width - mw - rw, mh * 2.f + th, rw, (float)height - 3.f * mh - th};
    layout_.top = {2.f * mw + lw, mh, (float)width - 4.f * mw - lw - rw, th};
    layout_.bottom = {2.f * mw + lw, (float)height - bh - mh, (float)width - 4.f * mw - lw - rw, bh};
    layout_.status = {2.f * mw + lw, th + 2.f * mh, 180, 90};
}

void Application::onFramebufferSizeChanged(int width, int height) {
    renderer_->onFramebufferSizeChanged(width, height);
}

RpcClient::State Application::updateServerParams() {
    for (auto comp : components_) {
        CHECK_CLIENT_STATE(comp->updateServerParams())
    }
    return RpcClient::State::OK;
}

void Application::startConsumer() {
    running_ = true;
    consumer_thread_ = std::thread([&]() {
        auto& packets = rpc_client_->packets();
        RpcClient::DataType packet;
        while (running_) {
            if (!packets.tryPop(packet)) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            } else if (!consume(packet)) {
                spdlog::warn("Data ignored!");
            }
        }
    });
}

bool Application::consume(const RpcClient::DataType& packet) {
    if (std::holds_alternative<rpc::ProjectionData>(packet)) {
        const auto& data = std::get<rpc::ProjectionData>(packet);
        proj_comp_->setData(data.id(), data.data(), data.col_count(), data.row_count());
        projection_counter_.count();
        return true;
    }

    if (std::holds_alternative<rpc::ReconData>(packet)) {
        const auto& data = std::get<rpc::ReconData>(packet);

        if (data.has_slice()) {
            const auto& s_data = data.slice();
            if (slice_comp_->setData(s_data.timestamp(), s_data.data(), s_data.col_count(), s_data.row_count())) {
                slice_counter_.count();
            }
            return true;
        }

        if (data.has_volume_shard()) {
            const auto& shard = data.volume_shard();
            if (volume_comp_->setShard(shard.pos(), shard.data(),
                                       shard.col_count(), shard.row_count(), shard.slice_count())) {
                volume_counter_.count();
            }
            return true;
        }

    }

    return false;
}

std::array<float, 2> Application::normalizeCursorPos(GLFWwindow* window, double xpos, double ypos) {
    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);

    xpos = (2.0 * (xpos / w) - 1.0) * w / h;
    ypos = 2.0 * (ypos / h) - 1.0;
    return {static_cast<float>(xpos), static_cast<float>(ypos)};
}

void Application::initRenderer() {

    renderer_ = std::make_unique<Renderer>();
    renderer_->init(glfw_window_);

    renderer_->addViewport(
            VIEWPORT_MAIN,
            2.f * Style::MARGIN + Style::LEFT_PANEL_WIDTH,
            2.f * Style::MARGIN + Style::TOP_PANEL_HEIGHT,
            1.f - 4.f * Style::MARGIN - Style::LEFT_PANEL_WIDTH - Style::RIGHT_PANEL_WIDTH,
            1.f - 4.f * Style::MARGIN - Style::TOP_PANEL_HEIGHT - Style::BOTTOM_PANEL_HEIGHT,
            Viewport::Type::PERSPECTIVE
    );

    renderer_->addViewport(
            VIEWPORT_SATELLITE,
            1.f - 2 * Style::MARGIN - Style::RIGHT_PANEL_WIDTH - Style::SATELLITE_WINDOW_SIZE,
            2 * Style::MARGIN + Style::BOTTOM_PANEL_HEIGHT,
            Style::SATELLITE_WINDOW_SIZE,
            Style::SATELLITE_WINDOW_SIZE,
            Viewport::Type::ORTHO
    );

    renderer_->addViewport(
            VIEWPORT_TOP_LEFT,
            Style::MARGIN,
            1.f - Style::MARGIN - Style::TOP_PANEL_HEIGHT,
            Style::LEFT_PANEL_WIDTH,
            Style::TOP_PANEL_HEIGHT,
            Viewport::Type::ORTHO
    );

    renderer_->addViewport(
            VIEWPORT_TOP_RIGHT,
            1.f - Style::MARGIN - Style::RIGHT_PANEL_WIDTH,
            1.f - Style::MARGIN - Style::TOP_PANEL_HEIGHT,
            Style::RIGHT_PANEL_WIDTH,
            Style::TOP_PANEL_HEIGHT,
            Viewport::Type::ORTHO
    );
}

void Application::initCenterScene() {
    scenes_.push_back(std::make_unique<Scene>());
    auto scene = scenes_.back().get();
    scene->setViewport(VIEWPORT_MAIN);

    auto light1 = scene->addLight();
    light1->setDirection({-1, -1, -1});
    light1->setColor({1.f, 1.f, 1.f});
    light1->setAmbient(0.2f);
    light1->setDiffuse(0.8f);
    light1->setSpecular(0.8f);

    auto light2 = scene->addLight();
    light2->setDirection({1, 1, 1});
    light2->setColor({1.f, 1.f, 1.f});
    light2->setAmbient(0.2f);
    light2->setDiffuse(0.8f);
    light2->setSpecular(0.8f);

    auto voxel_obj = scene->addObject<VoxelObject>();
    auto voxel_mat = MaterialManager::instance().createMaterial<TransferFunc>();
    voxel_obj->setMaterial(voxel_mat->id());

    auto mesh_obj = scene->addObject<MeshObject>(ShapeType::CUBE);
    auto mesh_mat = MaterialManager::instance().createMaterial<MeshMaterial>();
    mesh_mat->setAmbient(glm::vec3{0.24725f, 0.1995f, 0.0745f});
    mesh_mat->setDiffuse(glm::vec3{0.75164f, 0.60648f, 0.22648f});
    mesh_mat->setSpecular(glm::vec3{0.628281f, 0.555802f, 0.366065f});
    mesh_mat->setShininess(51.2f);
    mesh_obj->setMaterial(mesh_mat->id());

    volume_comp_->setMeshObject(mesh_obj);
    volume_comp_->setVoxelObject(voxel_obj);

    for (size_t i = 0; i < MAX_NUM_SLICES; ++i) {
        auto slice_obj = scene->addObject<SliceObject>();
        slice_obj->setMaterial(voxel_mat->id());
        slice_obj->setVoxelIntensity(voxel_obj->intensity());
        slice_comp_->addSliceObject(slice_obj);
    }

    slice_comp_->setVolumeComponent(volume_comp_.get());
    volume_comp_->setSliceComponent(slice_comp_.get());

    scene->addObject<SimpleObject>(SimpleObject::Type::CUBE_FRAME);
}

void Application::initTopLeftScene() {
    scenes_.push_back(std::make_unique<Scene>());
    auto scene = scenes_.back().get();
    scene->setViewport(VIEWPORT_TOP_LEFT);
    scene->setCameraFixed(true);
    auto glyph = scene->addObject<GlyphObject>(std::string("RECASTX"));
    glyph->setScale({0.9f, 0.9f, 1.0});
    glyph->setColor({0.7f, 0.8f, 0.2f});
}

void Application::initTopRightScene() {
    scenes_.push_back(std::make_unique<Scene>());
    auto scene = scenes_.back().get();
    scene->setViewport(VIEWPORT_TOP_RIGHT);
    scene->setCamera(scenes_[0]->camera());
    scene->setCameraFixed(true);
    scene->addObject<SimpleObject>(SimpleObject::Type::AXIS_CUBE);
}

void Application::initSatelliteScene() {
    scenes_.push_back(std::make_unique<Scene>());
    auto scene = scenes_.back().get();
    scene->camera()->setTopView();
    scene->setCameraFixed(true);
    scene->setViewport(VIEWPORT_SATELLITE);
    auto image = scene->addObject<ImageObject>();
    auto mat = MaterialManager::instance().createMaterial<TransferFunc>();
    mat->setAlphaEnabled(false);
    image->setMaterial(mat->id());
    proj_comp_->setImageObject(image);
}

void Application::drawLeftGUI() {
    ImGui::SetNextWindowPos({layout_.left[0], layout_.left[1]});
    ImGui::SetNextWindowSize({layout_.left[2], layout_.left[3]});
    ImGui::Begin("Control Panel Left", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);
    drawMainControlGUI();
    ImGui::Separator();
    scan_comp_->draw(server_state_);
    ImGui::Separator();
    scenes_[0]->drawCameraControlGUI();
    ImGui::Separator();
    geom_comp_->draw(server_state_);
    ImGui::Separator();
    proj_comp_->draw(server_state_);
    ImGui::Separator();
    preproc_comp_->draw(server_state_);
    ImGui::End();
}

void Application::drawRightGUI() {
    ImGui::SetNextWindowPos({layout_.right[0], layout_.right[1]});
    ImGui::SetNextWindowSize({layout_.right[2], layout_.right[3]});
    ImGui::Begin("Control Panel Right", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);

    slice_comp_->draw(server_state_);
    volume_comp_->draw(server_state_);

    scenes_[0]->drawLightControlGUI();
    ImGui::End();
}

void Application::drawTopGUI() {
    ImGui::SetNextWindowPos({layout_.top[0], layout_.top[1]});
    ImGui::SetNextWindowSize({layout_.top[2], layout_.top[3]});
    ImGui::Begin("Control Panel Top", NULL, ImGuiWindowFlags_NoDecoration);
    slice_comp_->drawStatistics(server_state_);
    ImGui::End();
}

void Application::drawBottomGUI() {
    ImGui::SetNextWindowPos({layout_.bottom[0], layout_.bottom[1]});
    ImGui::SetNextWindowSize({layout_.bottom[2], layout_.bottom[3]});
    ImGui::Begin("Control Panel Bottom", NULL, ImGuiWindowFlags_NoDecoration);
    log_comp_->draw(server_state_);
    ImGui::End();
}

void Application::drawStatusGUI() {
    ImGui::SetNextWindowPos({layout_.status[0], layout_.status[1]});
    ImGui::SetNextWindowSize({layout_.status[2], layout_.status[3]});

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));

    ImGui::Begin("Status Panel", NULL, ImGuiWindowFlags_NoDecoration);

    auto& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Volume: %.1f Hz", volume_counter_.frameRate());
    ImGui::Text("Slice: %.1f Hz", slice_counter_.frameRate());
    ImGui::Text("Projection: %.1f Hz", projection_counter_.frameRate());

    ImGui::PopStyleColor(2);

    ImGui::End();
}

void Application::drawMainControlGUI() {
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.2f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.2f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.2f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State_UNKNOWN);
    if (ImGui::Button("Connect")) connectServer();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State_READY);
    if (ImGui::Button("Acquire")) startAcquiring();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State_READY);
    if (ImGui::Button("Process")) startProcessing();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    ImGui::BeginDisabled(!(server_state_ & rpc::ServerState_State_PROCESSING));
    if (ImGui::Button("Stop")) {
        if (server_state_ == rpc::ServerState_State_ACQUIRING) stopAcquiring();
        else if (server_state_ == rpc::ServerState_State_PROCESSING) stopProcessing();
        else log::warn("Stop from unexpected server state: {}", RpcClient::serverStateToString(server_state_));
    };
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    ImGui::PopItemWidth();
}

} // namespace recastx::gui
