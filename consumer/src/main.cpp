#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
 
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "server/server.hpp"

int main() {
    // start the server
    tomovis::Server server;

    server.start();

    while (true) { std::this_thread::sleep_for(std::chrono::seconds(1)); }

    return 0;
}
