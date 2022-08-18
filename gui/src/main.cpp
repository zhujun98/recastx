#include <iostream>

#include <boost/program_options.hpp>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include "application.hpp"
#include "server.hpp"
#include "graphics/scenes/scene3d.hpp"


int main(int argc, char** argv) {
    using namespace tomcat::gui;
    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "print help message")
        ("port", po::value<int>()->default_value(9970),
         "ZMQ socket port of the consumer")
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

    Server server(opts["port"].as<int>());

    app.setScene(&scene);
    app.setPublisher(&server);

    server.start();
    app.start();

    return 0;
}
