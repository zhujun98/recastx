#include <iostream>

#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include "application.hpp"
#include "graphics/scene3d.hpp"
#include "client.hpp"

int main(int argc, char** argv) {
    using namespace tomcat::gui;
    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "print help message")
        ("recon-host", po::value<std::string>()->default_value("localhost"),
         "hostname of the reconstruction server")
        ("recon-port", po::value<int>()->default_value(9970),
         "First ZMQ socket port of the GUI server. The second port has an increment of 1. "
         "The valid port range is [9970, 9979]")
    ;

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    spdlog::set_level(spdlog::level::info);

    auto& app = Application::instance();

    Client client(opts["recon-host"].as<std::string>(), opts["recon-port"].as<int>());
    Scene3d scene(&client);
    app.setScene(&scene);

    scene.init();
    client.start();

    app.exec();

    spdlog::info("GUI application closed!");
    return 0;
}
