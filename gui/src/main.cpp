/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <iostream>

#include <boost/program_options.hpp>

#include "application.hpp"

int main(int argc, char** argv) {
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    using namespace recastx::gui;
    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "print help message")
        ("server", po::value<std::string>()->default_value("localhost:9971"),
         "address (<hostname>:<port>) of the reconstruction server")
    ;

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    auto& app = Application::instance();
    app.spin(opts["server"].as<std::string>());

    spdlog::info("GUI application closed!");
    return 0;
}
