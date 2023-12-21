/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "application.hpp"
#include "rpc_server.hpp"

int main() {
    using namespace recastx::gui::test;

    Application app;

    RpcServer server(9971, &app);
    server.start();
}
