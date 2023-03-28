#include <iostream>

#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include "application.hpp"
#include "graphics/scene3d.hpp"
#include "zmq_client.hpp"

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
        ("recon-host", po::value<std::string>()->default_value("localhost"),
         "hostname of the reconstruction server")
        ("data-port", po::value<int>()->default_value(9970),
         "ZMQ socket port of the reconstruction data server. ")
        ("message-port", po::value<int>()->default_value(9971),
         "ZMQ socket port of the reconstruction message server. ")
    ;

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    auto& app = Application::instance();

    Scene3d scene;
    DataClient data_client(opts["recon-host"].as<std::string>(), opts["data-port"].as<int>());
    MessageClient msg_client(opts["recon-host"].as<std::string>(), opts["message-port"].as<int>());
    scene.setClient(&msg_client);
    app.setScene(&scene);

    scene.init();
    data_client.start();

    app.exec();

    spdlog::info("GUI application closed!");
    return 0;
}
