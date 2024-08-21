/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_PROJECTION_COMPONENT_H
#define GUI_PROJECTION_COMPONENT_H

#include <common/config.hpp>

#include "component.hpp"
#include "data.hpp"

namespace recastx::gui {

class ImageObject;

class ProjectionComponent : public Component {

    int id_ {0};
    static constexpr int K_MAX_ID_ = 10000;
    int displayed_id_ {0};

    std::mutex mtx_;
    Data2D<RawDtype> data_;

    bool display_ { true };

    ImageObject* image_object_;
    bool update_texture_ { true };

  public:

    explicit ProjectionComponent(RpcClient* client);

    ~ProjectionComponent() override;

    void setImageObject(ImageObject* obj) { image_object_ = obj; }

    void draw(rpc::ServerState_State) override;

    [[nodiscard]] RpcClient::State updateServerParams() const override;

    void setData(uint32_t id, const std::string& data, uint32_t x, uint32_t y);

    void preRender() override;
};

}  // namespace recastx::gui

#endif // GUI_PROJECTION_COMPONENT_H