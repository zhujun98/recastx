/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SLICE_H
#define GUI_SLICE_H

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

#include <glm/glm.hpp>

#include "component.hpp"
#include "textures.hpp"
#include "data.hpp"
#include "common/config.hpp"

namespace recastx::gui {

class SliceObject;
class VolumeComponent;

class SliceComponent : public Component {

public:

    enum class Plane { XY, YZ, XZ };
    enum DisplayPolicy { SHOW = 0, PREVIEW = 1, DISABLE = 2 };

    struct Slice {
        uint32_t id;
        uint64_t timestamp;
        SliceObject *object = nullptr;
        float offset = 0;
        Data2D<ProDtype> data;
        bool update_texture = false;
        bool dragging = false;
        Plane plane;
        int display_policy = SHOW;
    };

private:

    std::vector<Slice> slices_;
    mutable std::mutex mtx_;

    static constexpr glm::vec4 K_EMPTY_FRAME_COLOR_{1.0f, 1.0f, 1.0f, 0.5f};
    static constexpr glm::vec4 K_HIGHLIGHTED_FRAME_COLOR_{0.8f, 0.8f, 0.0f, 1.f};

    VolumeComponent* volume_comp_ { nullptr };

    void renderSliceControl(size_t index, const char *header);

    void reset(Slice &slice);

public:

    explicit SliceComponent(RpcClient *client);

    ~SliceComponent() override;

    void addSliceObject(SliceObject *obj);

    void preRender() override;

    void draw(rpc::ServerState_State state) override;

    void drawStatistics(rpc::ServerState_State state);

    RpcClient::State updateServerParams() const override;

    bool setData(size_t timestamp, const std::string& data, uint32_t x, uint32_t y);

    void setVolumeComponent(VolumeComponent* ptr) { volume_comp_ = ptr; }

    void onShowVolume();

    void setSampleVolume(bool state);
};

} // namespace recastx::gui

#endif // GUI_SLICE_H