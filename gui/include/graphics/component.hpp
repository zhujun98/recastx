/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_COMPONENT_H
#define GUI_COMPONENT_H

#include <imgui.h>

#include "graphics/style.hpp"
#include "rpc_client.hpp"

namespace recastx::gui {

class Component {

  protected:

    RpcClient* client_;

  public:

    explicit Component(RpcClient* client) { client_ = client; };

    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    virtual void draw(rpc::ServerState_State state) = 0;

    virtual RpcClient::State updateServerParams() const { return RpcClient::State::OK; };

    virtual void preRender() {}
};

} // recastx::gui

#endif //GUI_COMPONENT_H
