#include "rpc_server.hpp"


int main() {
    recastx::gui::test::RpcServer server(9971);
    server.start();
}