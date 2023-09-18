/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_STDDAQCLIENT_H
#define RECON_STDDAQCLIENT_H

#include <string>

#include "zmq_daq_client.hpp"

namespace recastx::recon {

class StdDaqClient : public ZmqDaqClient {

  public:

    StdDaqClient(const std::string& endpoint, const std::string& socket_type, size_t max_concurrency = 1);

    ~StdDaqClient() override;

    std::optional<Projection<>> recv() override;
};

} // namespace recastx::recon

#endif // RECON_STDDAQCLIENT_H