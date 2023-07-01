/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQCLIENT_H
#define RECON_DAQCLIENT_H

#include <string>
#include <thread>

#include <zmq.hpp>

namespace recastx::recon {

class Application;

class DaqClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

    Application* app_;

    zmq::socket_type parseSocketType(const std::string& socket_type) const; 

    bool running_ = false;
    bool acquiring_ = false;

    bool initialized_ = false;
    size_t num_rows_;
    size_t num_cols_;

public:

    DaqClient(const std::string& endpoint,
              const std::string& socket_type,
              Application* app);

    ~DaqClient();

    void start();

    void stopAcquiring();
    void startAcquiring();
};


} // namespace recastx::recon

#endif // RECON_DAQCLIENT_H