/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECASTX_SCAN_COMPONENT_HPP
#define RECASTX_SCAN_COMPONENT_HPP

#include <map>
#include <string>

#include "component.hpp"

namespace recastx::gui {

class ScanComponent : public Component {

    rpc::ScanMode_Mode scan_mode_;
    uint32_t scan_update_interval_;

  public:

    explicit ScanComponent(RpcClient* client);

    ~ScanComponent() override;

    void draw(rpc::ServerState_State state) override;

    RpcClient::State updateServerParams() const override;
};

} // namespace recastx::gui

#endif //RECASTX_SCAN_COMPONENT_HPP
