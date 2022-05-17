#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
 
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <boost/program_options.hpp>

#include "server/server.hpp"

int main(int argc, char** argv) {
    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "print help message")
        ("port", po::value<int>()->default_value(5555),
         "ZMQ socket port of the consumer")
    ;

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    // start the server
    tomovis::Server server(opts["port"].as<int>());

    server.start();

    while (true) { std::this_thread::sleep_for(std::chrono::seconds(1)); }

    return 0;
}
