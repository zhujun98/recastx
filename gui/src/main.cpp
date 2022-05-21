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
#include "scene_list.hpp"
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
    gui::Interface interface(renderer.window());
    gui::SceneList scenes;
    gui::SceneSwitcher scene_switcher(scenes);
    gui::SceneControl scene_control(scenes);
    gui::Server server(scenes, opts["port"].as<int>());
    server.register_module(std::make_shared<gui::ManageSceneProtocol>());
    server.register_module(std::make_shared<gui::ReconstructionProtocol>());
    server.register_module(std::make_shared<gui::GeometryProtocol>());
    server.register_module(std::make_shared<gui::PartitioningProtocol>());
    server.register_module(std::make_shared<gui::ControlProtocol>());

    interface.register_window(scene_switcher);
    interface.register_window(scene_control);

    input.register_handler(interface);
    input.register_handler(scenes);
    input.register_handler(scene_switcher);

    renderer.register_ticker(input);
    renderer.register_ticker(scenes);
    renderer.register_target(interface);
    renderer.register_target(scenes);
    renderer.register_ticker(server);

    server.start();

    renderer.main_loop();

    return 0;
}
