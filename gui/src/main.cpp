#include <iostream>
#include <memory>
#include <thread>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <boost/program_options.hpp>

#include "graphics/interface/interface.hpp"
#include "graphics/interface/scene_control.hpp"
#include "graphics/interface/scene_switcher.hpp"
#include "graphics/renderer.hpp"
#include "graphics/scene_camera.hpp"
#include "input.hpp"
#include "modules/geometry.hpp"
#include "modules/partitioning.hpp"
#include "modules/reconstruction.hpp"
#include "modules/scene_management.hpp"
#include "modules/control.hpp"
#include "scene.hpp"
#include "scene_list.hpp"
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

    tomovis::Renderer renderer;

    auto& input = tomovis::Input::instance(renderer.window());
    renderer.register_ticker(input);

    // construct interface
    tomovis::Interface interface(renderer.window());
    renderer.register_target(interface);
    input.register_handler(interface);

    // construct the scenes
    tomovis::SceneList scenes;
    renderer.register_target(scenes);
    renderer.register_ticker(scenes);
    input.register_handler(scenes);

    tomovis::SceneSwitcher scene_switcher(scenes);
    interface.register_window(scene_switcher);
    input.register_handler(scene_switcher);

    tomovis::SceneControl scene_control(scenes);
    interface.register_window(scene_control);

    // start the server
    tomovis::Server server(scenes, opts["port"].as<int>());

    // add more modules to the server
    server.register_module(std::make_shared<tomovis::ManageSceneProtocol>());
    server.register_module(std::make_shared<tomovis::ReconstructionProtocol>());
    server.register_module(std::make_shared<tomovis::GeometryProtocol>());
    server.register_module(std::make_shared<tomovis::PartitioningProtocol>());
    server.register_module(std::make_shared<tomovis::ControlProtocol>());

    server.start();
    renderer.register_ticker(server);

    // enter main loop
    renderer.main_loop();

    return 0;
}
