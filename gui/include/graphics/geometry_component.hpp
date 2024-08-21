/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_GEOMETRY_COMPONENT_H
#define GUI_GEOMETRY_COMPONENT_H

#include <common/config.hpp>

#include "component.hpp"

namespace recastx::gui {

class GeometryComponent : public Component {

    int beam_shape_;

    int col_count_;
    int row_count_;

    int angle_count_;
    int angle_range_;

    int slice_size_;
    int volume_size_;

    int x_[2];
    int y_[2];
    int z_[2];

public:

    explicit GeometryComponent(RpcClient* client);

    ~GeometryComponent() override;

    void draw(rpc::ServerState_State) override;

    RpcClient::State updateServerParams() const override;
};

}  // namespace recastx::gui

#endif // GUI_GEOMETRY_COMPONENT_H