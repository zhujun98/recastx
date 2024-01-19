/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
