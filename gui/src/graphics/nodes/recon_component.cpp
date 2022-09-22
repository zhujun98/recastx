#include <iostream>

#include <spdlog/spdlog.h>

#include "glm/gtc/constants.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "implot.h"

#include "tomcat/tomcat.hpp"

#include "graphics/nodes/recon_component.hpp"
#include "graphics/nodes/camera.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/primitives.hpp"
#include "graphics/style.hpp"
#include "util.hpp"

namespace tomcat::gui {

ReconComponent::ReconComponent(Scene& scene) : DynamicSceneComponent(scene) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), primitives::square,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &line_vao_);
    glBindVertexArray(line_vao_);
    glGenBuffers(1, &line_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo_);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), primitives::line, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &cube_vao_);
    glBindVertexArray(cube_vao_);
    cube_index_count_ = 24;
    glGenBuffers(1, &cube_index_handle_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_index_handle_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_index_count_ * sizeof(GLuint),
                 primitives::cube_wireframe_idxs, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &cube_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float),
                 primitives::cube_wireframe, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    auto solid_vert =
#include "../shaders/solid_cube.vert"
        ;
    auto solid_frag =
#include "../shaders/solid_cube.frag"
        ;
    solid_shader_ = std::make_unique<ShaderProgram>(solid_vert, solid_frag);

    auto wireframe_vert =
#include "../shaders/wireframe_cube.vert"
        ;
    auto wireframe_frag =
#include "../shaders/wireframe_cube.frag"
        ;
    wireframe_shader_ = std::make_unique<ShaderProgram>(wireframe_vert, wireframe_frag);

    initSlices();
    initVolume();
    maybeUpdateMinMaxValues();
}

ReconComponent::~ReconComponent() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);

    glDeleteVertexArrays(1, &cube_vao_);
    glDeleteBuffers(1, &cube_vbo_);
    glDeleteBuffers(1, &cube_index_handle_);

    glDeleteVertexArrays(1, &line_vao_);
    glDeleteBuffers(1, &line_vbo_);
}

void ReconComponent::onWindowSizeChanged(int width, int height) {
    pos_ = {
        Style::IMGUI_WINDOW_MARGIN + Style::IMGUI_CONTROL_PANEL_WIDTH + Style::IMGUI_WINDOW_SPACING,
        Style::IMGUI_WINDOW_MARGIN
    };
    size_ = {
        static_cast<float>(width) - pos_[0]
            - Style::IMGUI_WINDOW_MARGIN - Style::IMGUI_WINDOW_SPACING - Style::IMGUI_ROTATING_AXIS_WIDTH,
        Style::IMGUI_TOP_PANEL_HEIGHT
    };
}

void ReconComponent::renderIm() {
    cm_.renderIm();

    ImGui::Checkbox("Auto Levels", &auto_levels_);

    float step_size = (max_val_ - min_val_) / 100.f;
    if (step_size < 0.01f) step_size = 0.01f; // avoid a tiny step size
    ImGui::DragFloatRange2("Min / Max", &min_val_, &max_val_, step_size,
                           std::numeric_limits<float>::lowest(), // min() does not work
                           std::numeric_limits<float>::max());

    if(ImGui::Button("Reset slices")) {
        initSlices();
        init();
    }

    ImGui::Checkbox("Show slice histograms", &show_statistics_);
    if (show_statistics_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Statistics##ReconComponent", NULL, ImGuiWindowFlags_NoDecoration);

        ImPlot::BeginSubplots("##Histograms", 1, 3, ImVec2(-1.f, -1.f));
        for (auto &[slice_id, slice]: slices_) {
            const auto &data = slice->data();
            // FIXME: faster way to build the title?
            if (ImPlot::BeginPlot(("Slice " + std::to_string(slice_id)).c_str(),
                                  ImVec2(-1.f, -1.f))) {
                ImPlot::SetupAxes("Pixel value", "Density",
                                  ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::PlotHistogram("##Histogram", data.data(), static_cast<int>(data.size()),
                                      100, false, true);
                ImPlot::EndPlot();
            }
        }
        ImPlot::EndSubplots();

        ImGui::End();
    }
}

void ReconComponent::renderGl() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    solid_shader_->use();
    solid_shader_->setInt("colormap", 0);
    solid_shader_->setInt("sliceData", 1);
    solid_shader_->setInt("volumeData", 2);
    solid_shader_->setFloat("minValue", min_val_);
    solid_shader_->setFloat("maxValue", max_val_);

    cm_.colormap().bind();

    glm::mat4 view_matrix = scene_.camera().matrix() * volume_transform_;

    std::vector<Slice*> slices;
    for (auto& [slice_id, slice] : slices_) {
        if (slice->inactive()) continue;
        slices.push_back(slice.get());
    }
    std::sort(slices.begin(), slices.end(), [](auto& lhs, auto& rhs) -> bool {
        if (rhs->transparent() == lhs->transparent()) {
            return rhs->id() < lhs->id();
        }
        return rhs->transparent();
    });

    // FIXME: why do we need to bind and unbind 3D texture?
    volume_->bind();
    for (auto slice : slices) drawSlice(slice, view_matrix);
    volume_->unbind();

    cm_.colormap().unbind();

    wireframe_shader_->use();
    wireframe_shader_->setMat4("view", view_matrix);
    wireframe_shader_->setMat4("projection", scene_.projection());
    wireframe_shader_->setVec4("color", glm::vec4(1.f, 1.f, 1.f, 0.2f));

    glBindVertexArray(cube_vao_);
    glLineWidth(3.f);
    glDrawElements(GL_LINES, cube_index_count_, GL_UNSIGNED_INT, nullptr);

    glDisable(GL_DEPTH_TEST);

    if (drag_machine_ != nullptr && drag_machine_->type() == DragType::rotator) {
        auto& rotator = *(SliceRotator*)drag_machine_.get();
        wireframe_shader_->setMat4(
                "view", view_matrix * glm::translate(rotator.rot_base) * glm::scale(rotator.rot_end - rotator.rot_base));
        wireframe_shader_->setVec4("color", glm::vec4(1.f, 1.f, 1.f, 1.f));
        glBindVertexArray(line_vao_);
        glLineWidth(10.f);
        glDrawArrays(GL_LINES, 0, 2);
    }

    glDisable(GL_BLEND);
}

void ReconComponent::init() {
    scene_.send(RemoveAllSlicesPacket());

    for (auto& slice : slices_) {
        scene_.send(SetSlicePacket(slice.first, slice.second->orientation3()));
    }
}

void ReconComponent::setSliceData(std::vector<float>&& data,
                                  const std::array<uint32_t, 2>& size,
                                  int slice_idx) {
    Slice* slice;
    if (slices_.find(slice_idx) != slices_.end()) {
        slice = slices_[slice_idx].get();
    } else {
        std::cout << "Updating inactive slice: " << slice_idx << "\n";
        return;
    }

    if (slice == dragged_slice_) return;

    // FIXME: replace uint32_t with size_t in Packet
    slice->setData(std::move(data), {size[0], size[1]});
    maybeUpdateMinMaxValues();
}

void ReconComponent::setVolumeData(std::vector<float>&& data, const std::array<uint32_t, 3>& size) {
    // FIXME: replace uint32_t with size_t in Packet
    volume_->setData(std::move(data), {size[0], size[1], size[2]});
    maybeUpdateMinMaxValues();
}

bool ReconComponent::consume(const tomcat::PacketDataEvent &data) {
    switch (data.first) {
        case PacketDesc::slice_data: {
            auto packet = dynamic_cast<SliceDataPacket*>(data.second.get());
            setSliceData(std::move(packet->data), packet->slice_size, packet->slice_id);
            spdlog::info("Set slice data {}", packet->slice_id);
            return true;
        }
        case PacketDesc::volume_data: {
            auto packet = dynamic_cast<VolumeDataPacket*>(data.second.get());
            setVolumeData(std::move(packet->data), packet->volume_size);
            spdlog::info("Set volume data");
            return true;
        }
        default: {
            return false;
        }
    }
}

bool ReconComponent::handleMouseButton(int button, int action) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (hovered_slice_ != nullptr) {
                maybeSwitchDragMachine(DragType::translator);
                dragged_slice_ = hovered_slice_;

#if (VERBOSITY >= 4)
                spdlog::info("Set dragged slice: {}", dragged_slice_->id());
#endif

                return true;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (hovered_slice_ != nullptr) {
                maybeSwitchDragMachine(DragType::rotator);
                dragged_slice_ = hovered_slice_;

#if (VERBOSITY >= 4)
                spdlog::info("Set dragged slice: {}", dragged_slice_->id());
#endif

                return true;
            }
        }
    } else if (action == GLFW_RELEASE) {
        if (dragged_slice_ != nullptr) {
            auto packet = SetSlicePacket(dragged_slice_->id(), dragged_slice_->orientation3());
            scene_.send(packet);

            dragged_slice_ = nullptr;
            drag_machine_ = nullptr;
            return true;
        }
        drag_machine_ = nullptr;
    }

    return false;
}

bool ReconComponent::handleMouseMoved(double x, double y) {
    // update slice that is being hovered over
    y = -y;

    if (prev_y_ < -1.0) {
        prev_x_ = x;
        prev_y_ = y;
    }

    glm::vec2 delta(x - prev_x_, y - prev_y_);
    prev_x_ = x;
    prev_y_ = y;

    // TODO: fix for screen ratio
    if (dragged_slice_ != nullptr) {
        drag_machine_->onDrag(delta);
        return true;
    }

    updateHoveringSlice(x, y);

    return false;
}

void ReconComponent::initSlices() {
    for (int i = 0; i < 3; ++i) slices_[i] = std::make_unique<Slice>(i);

    resetSlices();
}

void ReconComponent::resetSlices() {
    // slice along axis 0 = x
    slices_[0]->setOrientation(glm::vec3(0.0f, -1.0f, -1.0f),
                               glm::vec3(0.0f, 2.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 2.0f));

    // slice along axis 1 = y
    slices_[1]->setOrientation(glm::vec3(-1.0f, 0.0f, -1.0f),
                               glm::vec3(2.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 2.0f));

    // slice along axis 2 = z
    slices_[2]->setOrientation(glm::vec3(-1.0f, -1.0f, 0.0f),
                               glm::vec3(2.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 2.0f, 0.0f));
}

void ReconComponent::initVolume() {
    volume_ = std::make_unique<Volume>();

    glm::vec3 min_pt(-1.0f), max_pt(1.0f);
    auto center = 0.5f * (min_pt + max_pt);
    volume_transform_ = glm::translate(center) *
                        glm::scale(glm::vec3(max_pt - min_pt)) *
                        glm::scale(glm::vec3(0.5f));
    scene_.camera().lookAt(center);
}

void ReconComponent::updateHoveringSlice(float x, float y) {
    auto inv_matrix = glm::inverse(scene_.projection() * scene_.camera().matrix() * volume_transform_);
    int slice_id = -1;
    float best_z = std::numeric_limits<float>::max();
    for (auto& [sid, slice] : slices_) {
        if (slice->inactive()) continue;
        slice->setHovered(false);
        auto maybe_point = intersectionPoint(inv_matrix, slice->orientation4(), glm::vec2(x, y));
        if (std::get<0>(maybe_point)) {
            auto z = std::get<1>(maybe_point);
            if (z < best_z) {
                best_z = z;
                slice_id = sid;
            }
        }
    }

    if (slice_id >= 0) {
        slices_[slice_id]->setHovered(true);
        hovered_slice_ = slices_[slice_id].get();
    } else {
        hovered_slice_ = nullptr;
    }
}

void ReconComponent::maybeSwitchDragMachine(ReconComponent::DragType type) {
    if (drag_machine_ == nullptr || drag_machine_->type() != type) {
        switch (type) {
            case DragType::translator:
                drag_machine_ = std::make_unique<SliceTranslator>(
                        *this, glm::vec2{prev_x_, prev_y_});
                break;
            case DragType::rotator:
                drag_machine_ = std::make_unique<SliceRotator>(
                        *this, glm::vec2{prev_x_, prev_y_});
                break;
            default:
                break;
        }
    }
}

void ReconComponent::drawSlice(Slice* slice, const glm::mat4& view_matrix) {
    // FIXME: bind an empty slice will result in warning:
    //        UNSUPPORTED (log once): POSSIBLE ISSUE: unit 1 GLD_TEXTURE_INDEX_2D is
    //        unloadable and bound to sampler type (Float) - using zero texture because texture unloadable
    slice->bind();

    solid_shader_->setMat4("view", view_matrix);
    solid_shader_->setMat4("projection", scene_.projection());
    solid_shader_->setMat4("orientationMatrix",
                      slice->orientation4() * glm::translate(glm::vec3(0.0, 0.0, 1.0)));
    solid_shader_->setBool("hovered", slice->hovered());
    solid_shader_->setBool("empty", slice->empty());

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    slice->unbind();
}

void ReconComponent::maybeUpdateMinMaxValues() {
    if (!auto_levels_) return;

    auto overall_min = std::numeric_limits<float>::max();
    auto overall_max = std::numeric_limits<float>::min();

    for (auto&& [slice_idx, slice] : slices_) {
        (void)slice_idx;

        auto [min_v, max_v] = slice->minMaxVals();
        overall_min = min_v < overall_min ? min_v : overall_min;
        overall_max = max_v > overall_max ? max_v : overall_max;
    }

    auto [min_v, max_v] = volume_->minMaxVals();
    min_val_ = min_v < overall_min ? min_v : overall_min;
    max_val_ = max_v > overall_max ? max_v : overall_max;
}

// ReconComponent::DragMachine

ReconComponent::DragMachine::DragMachine(ReconComponent& comp, const glm::vec2& initial, DragType type)
    : comp_(comp), initial_(initial), type_(type) {}

ReconComponent::DragMachine::~DragMachine() = default;

// ReconComponent::SliceTranslator

ReconComponent::SliceTranslator::SliceTranslator(ReconComponent &comp, const glm::vec2& initial)
    : DragMachine(comp, initial, DragType::translator) {}

ReconComponent::SliceTranslator::~SliceTranslator() = default;

void ReconComponent::SliceTranslator::onDrag(glm::vec2 delta) {
    // 1) what are we dragging, and does it have data?
    // if it does then we need to make a new slice
    // else we drag the current slice along the normal
    if (!comp_.dragged_slice()) {
        std::unique_ptr<Slice> new_slice;
        int id = comp_.generate_slice_idx();
        int to_remove = -1;
        for (auto& id_the_slice : comp_.slices()) {
            auto& the_slice = id_the_slice.second;
            if (the_slice->hovered()) {
                if (!the_slice->empty()) {
                    new_slice = std::make_unique<Slice>(id);
                    new_slice->setOrientation(the_slice->orientation4());
                    to_remove = the_slice->id();
                    // FIXME need to generate a new id and upon 'popping'
                    // send a UpdateSlice packet
                    comp_.dragged_slice() = new_slice.get();
                } else {
                    comp_.dragged_slice() = the_slice.get();
                }
                break;
            }
        }
        if (new_slice) {
            comp_.slices()[new_slice->id()] = std::move(new_slice);
        }
        if (to_remove >= 0) {
            comp_.slices().erase(to_remove);
            // send slice packet
            auto packet = RemoveSlicePacket(to_remove);
            comp_.scene().send(packet);
        }
        if (!comp_.dragged_slice()) {
            std::cout << "WARNING: No dragged slice found." << std::endl;
            return;
        }
    }

    auto slice = comp_.dragged_slice();
    auto& o = slice->orientation4();

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto normal = glm::normalize(glm::cross(axis1, axis2));

    // project the normal vector to screen coordinates
    // FIXME maybe need window matrix here too which would be kind of
    // painful maybe
    auto base_point_normal =
        glm::vec3(o[2][0], o[2][1], o[2][2]) + 0.5f * (axis1 + axis2);
    auto end_point_normal = base_point_normal + normal;

    auto a = comp_.scene().projection() * comp_.scene().camera().matrix() * comp_.volume_transform() *
             glm::vec4(base_point_normal, 1.0f);
    auto b = comp_.scene().projection() * comp_.scene().camera().matrix() * comp_.volume_transform() *
             glm::vec4(end_point_normal, 1.0f);
    auto normal_delta = b - a;
    float difference =
        glm::dot(glm::vec2(normal_delta.x, normal_delta.y), delta);

    // take the inner product of delta x and this normal vector

    auto dx = difference * normal;
    // FIXME check if it is still inside the bounding box of the volume
    // probably by checking all four corners are inside bounding box, should
    // define this box somewhere
    o[2][0] += dx[0];
    o[2][1] += dx[1];
    o[2][2] += dx[2];
}

// ReconComponent::SliceRotator

ReconComponent::SliceRotator::SliceRotator(ReconComponent& comp, const glm::vec2& initial)
    : DragMachine(comp, initial, DragType::rotator) {
    // 1. need to identify the opposite axis
    // a) get the position within the slice
    auto tf = comp.scene().projection() * comp.scene().camera().matrix() * comp.volume_transform();
    auto inv_matrix = glm::inverse(tf);

    auto slice = comp.hovered_slice();
    assert(slice);
    auto o = slice->orientation4();

    auto maybe_point = intersectionPoint(inv_matrix, o, initial_);
    assert(std::get<0>(maybe_point));

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);

    auto in_world = std::get<2>(maybe_point);
    auto rel = in_world - base;

    auto x = 0.5f * glm::dot(rel, axis1) - 1.0f;
    auto y = 0.5f * glm::dot(rel, axis2) - 1.0f;

    // 2. need to rotate around that at on drag
    auto other = glm::vec3();
    if (glm::abs(x) > glm::abs(y)) {
        if (x > 0.0f) {
            rot_base = base;
            rot_end = rot_base + axis2;
            other = axis1;
        } else {
            rot_base = base + axis1;
            rot_end = rot_base + axis2;
            other = -axis1;
        }
    } else {
        if (y > 0.0f) {
            rot_base = base;
            rot_end = rot_base + axis1;
            other = axis2;
        } else {
            rot_base = base + axis2;
            rot_end = rot_base + axis1;
            other = -axis2;
        }
    }

    auto center = 0.5f * (rot_end + rot_base);
    auto opposite_center = 0.5f * (rot_end + rot_base) + other;
    auto from = tf * glm::vec4(glm::rotate(rot_base - center,
                                           glm::half_pi<float>(), other) +
                                   opposite_center,
                               1.0f);
    auto to = tf * glm::vec4(glm::rotate(rot_end - center,
                                         glm::half_pi<float>(), other) +
                                 opposite_center,
                             1.0f);

    screen_direction = glm::normalize(from - to);
}

ReconComponent::SliceRotator::~SliceRotator() = default;

void ReconComponent::SliceRotator::onDrag(glm::vec2 delta) {
    // 1) what are we dragging, and does it have data?
    // if it does then we need to make a new slice
    // else we drag the current slice along the normal
    if (!comp_.dragged_slice()) {
        std::unique_ptr<Slice> new_slice;
        int id = comp_.generate_slice_idx();
        int to_remove = -1;
        for (auto& id_the_slice : comp_.slices()) {
            auto& the_slice = id_the_slice.second;
            if (the_slice->hovered()) {
                if (!the_slice->empty()) {
                    new_slice = std::make_unique<Slice>(id);
                    new_slice->setOrientation(the_slice->orientation4());
                    to_remove = the_slice->id();
                    // FIXME need to generate a new id and upon 'popping'
                    // send a UpdateSlice packet
                    comp_.dragged_slice() = new_slice.get();
                } else {
                    comp_.dragged_slice() = the_slice.get();
                }
                break;
            }
        }
        if (new_slice) {
            comp_.slices()[new_slice->id()] = std::move(new_slice);
        }
        if (to_remove >= 0) {
            comp_.slices().erase(to_remove);
            // send slice packet
            auto packet = RemoveSlicePacket(to_remove);
            comp_.scene().send(packet);
        }
        assert(comp_.dragged_slice());
    }

    auto slice = comp_.dragged_slice();
    auto& o = slice->orientation4();

    auto axis1 = glm::vec3(o[0][0], o[0][1], o[0][2]);
    auto axis2 = glm::vec3(o[1][0], o[1][1], o[1][2]);
    auto base = glm::vec3(o[2][0], o[2][1], o[2][2]);

    auto a = base - rot_base;
    auto b = base + axis1 - rot_base;
    auto c = base + axis2 - rot_base;

    auto weight = glm::dot(delta, screen_direction);
    a = glm::rotate(a, weight, rot_end - rot_base) + rot_base;
    b = glm::rotate(b, weight, rot_end - rot_base) + rot_base;
    c = glm::rotate(c, weight, rot_end - rot_base) + rot_base;

    slice->setOrientation(a, b - a, c - a);
}

} // tomcat::gui
