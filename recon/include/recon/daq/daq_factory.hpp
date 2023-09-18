/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQFACTORY_H
#define RECON_DAQFACTORY_H

#include <memory>
#include <string>

#include <spdlog/spdlog.h>

#include "std_daq_client.hpp"

namespace recastx::recon {

template<typename ...Ts>
std::unique_ptr<DaqClientInterface> createDaqClient(std::string_view protocol, Ts... args) {
    if (protocol == "default") {
      return std::make_unique<StdDaqClient>(std::forward<Ts>(args)...); 
    }

    throw std::runtime_error(fmt::format("Unknown DAQ client protocol: {}", protocol));
}

} // namespace recastx::recon

#endif // RECON_DAQFACTORY_H