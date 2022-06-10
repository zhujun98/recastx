#include <iostream>

#include <boost/program_options.hpp>

#include "graphics/interface.hpp"
#include "graphics/renderer.hpp"
#include "graphics/scene_camera.hpp"
#include "input.hpp"

#include "scene.hpp"
#include "server.hpp"


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

    gui::Renderer renderer;

    auto& input = gui::Input::instance(renderer.window());
    gui::SceneList scenes;
    gui::Interface interface(renderer.window(), scenes);
    gui::Server server(scenes, opts["port"].as<int>());

    input.register_handler(interface);
    input.register_handler(scenes);

    renderer.register_ticker(input);
    renderer.register_ticker(scenes);
    renderer.register_target(interface);
    renderer.register_target(scenes);
    renderer.register_ticker(server);

    server.start();

    renderer.main_loop();

    return 0;
}
