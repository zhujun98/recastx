/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECASTX_PREPROC_COMPONENT_HPP
#define RECASTX_PREPROC_COMPONENT_HPP

#include <map>
#include <string>

#include "component.hpp"

namespace recastx::gui {

class PreprocComponent : public Component {

    int downsampling_col_ = 1;
    int downsampling_row_ = 1;

    bool minus_log_ = true;
    int32_t offset_ = 0;

    std::string ramp_filter_name_;

    inline static const std::map<std::string, std::string> filter_options_ {
            {"shepp", "Shepp-Logan"},
            {"ramlak", "Ram-Lak"}
    };

  public:

    explicit PreprocComponent(RpcClient* client);

    ~PreprocComponent() override;

    void draw(rpc::ServerState_State state) override;

    RpcClient::State updateServerParams() const override;
};

} // namespace recastx::gui

#endif //RECASTX_PREPROC_COMPONENT_HPP
