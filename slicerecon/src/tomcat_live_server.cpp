#include <functional>
#include <numeric>
#include <string>

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "slicerecon/slicerecon.hpp"

using namespace std::string_literals;

zmq::socket_type parseSocketType(const std::string& socket_type) {
    if (socket_type.compare("pull") == 0) return zmq::socket_type::pull;
    if (socket_type.compare("sub") == 0) return zmq::socket_type::sub;
    throw std::invalid_argument("Unsupported socket type: "s + socket_type); 
}

int main(int argc, char** argv)
{
    namespace po = boost::program_options;

    po::options_description general_desc("General options");
    general_desc.add_options()
        ("help,h", "print help message")
    ;

    po::options_description connection_desc("ZMQ options");
    connection_desc.add_options()
        ("host", po::value<std::string>()->default_value("localhost"), 
         "hostname of the data source server")
        ("port", po::value<int>()->default_value(5558),
         "ZMQ socket port")
        ("socket", po::value<std::string>()->default_value("sub"),
         "ZMQ socket type")
        ("gui-host", po::value<std::string>()->default_value("localhost"),
         "hostname of the GUI server")
    ;

    po::options_description reconstruction_desc("Reconstruction options");
    bool continuous_mode = false;
    bool retrieve_phase = false;
    bool tilt = false;
    bool gaussian_pass = false;
    reconstruction_desc.add_options()
        ("slice-size", po::value<int>()->default_value(128),
         "...")
        ("preview-size", po::value<int>()->default_value(128),
         "...")
        ("group-size", po::value<int>()->default_value(128),
         "...")
        ("filter-cores", po::value<int>()->default_value(8),
         "number of CPU cores used by the filters")
        ("darks", po::value<int>()->default_value(10),
         "number of dark images")
        ("flats", po::value<int>()->default_value(10),
         "number of flat images")
        ("projections", po::value<int>()->default_value(128),
         "number of projections")
        ("continuous-mode", po::bool_switch(&continuous_mode),
         "switch reconstructor to continuous mode from alternating mode")
        ("retrieve-phase", po::bool_switch(&retrieve_phase),
         "...")
        ("tilt", po::bool_switch(&tilt),
         "...")
        ("gaussian", po::bool_switch(&gaussian_pass),
         "...")
        ("filter", po::value<std::string>()->default_value("shepp-logan"),
         "...")
    ;

    po::options_description geometry_desc("Geometry options");
    bool cone_beam = false;
    geometry_desc.add_options()
        ("rows", po::value<int>()->default_value(1200),
         "detector height in pixels")
        ("cols", po::value<int>()->default_value(2016),
         "detector width in pixels")
    ;

    po::options_description paganin_desc("Paganin options");
    paganin_desc.add_options()
        ("pixel-size", po::value<float>()->default_value(1.0f),
         "...")
        ("lambda", po::value<float>()->default_value(1.23984193e-9f),
         "...")
        ("delta", po::value<float>()->default_value(1.e-8f),
         "...")
        ("beta", po::value<float>()->default_value(1.e-10f),
         "...")
        ("distance", po::value<float>()->default_value(40.0f),
         "...")
    ;

    po::options_description misc_desc("Miscellaneous options");
    bool bench = false;
    bool plugin = false;
    bool py_plugin = false;
    misc_desc.add_options()
        ("bench", po::bool_switch(&bench),
         "...")
        ("plugin", po::bool_switch(&plugin),
         "...")
        ("py-plugin", po::bool_switch(&py_plugin),
         "...")
    ;

    po::options_description all_desc(
        "Allowed options for TOMCAT live 3D reconstruction server");
    all_desc.
        add(general_desc).
        add(connection_desc).
        add(reconstruction_desc).
        add(geometry_desc).
        add(paganin_desc).
        add(misc_desc);

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, all_desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << all_desc << "\n";
        return 0;
    }

    auto hostname = opts["host"].as<std::string>();
    auto port = opts["port"].as<int>();
    auto socket_type = parseSocketType(opts["socket"].as<std::string>());
    auto gui_hostname = opts["gui-host"].as<std::string>();

    auto slice_size = opts["slice-size"].as<int>();
    auto preview_size = opts["preview-size"].as<int>();
    auto group_size = opts["group-size"].as<int>();
    auto filter_cores = opts["filter-cores"].as<int>();
    auto darks = opts["darks"].as<int>();
    auto flats = opts["flats"].as<int>();
    auto recon_mode = continuous_mode ? slicerecon::ReconstructMode::continuous : 
                                        slicerecon::ReconstructMode::alternating;
    auto filter = opts["filter"].as<std::string>();

    auto rows = opts["rows"].as<int>();
    auto cols = opts["cols"].as<int>();
    auto projections = opts["projections"].as<int>();
    if (projections < group_size) {
        throw std::invalid_argument(
            "'projections' ("s + std::to_string(projections) + 
            ") is smaller than 'group_size' ("s + std::to_string(group_size) + ")"s);
    }

    auto pixel_size = opts["pixel-size"].as<float>();
    auto lambda = opts["lambda"].as<float>();
    auto delta = opts["delta"].as<float>();
    auto beta = opts["beta"].as<float>();
    auto distance = opts["distance"].as<float>();

    auto paganin = slicerecon::PaganinSettings{pixel_size, lambda, delta, beta, distance};

    auto params = slicerecon::Settings { slice_size, 
                                         preview_size, 
                                         group_size, 
                                         filter_cores, 
                                         darks, 
                                         flats, 
                                         projections,
                                         recon_mode, 
                                         false,
                                         retrieve_phase, 
                                         tilt, 
                                         paganin, 
                                         gaussian_pass, 
                                         filter };

    auto geom = slicerecon::Geometry { rows,
                                       cols,
                                       projections,
                                       {},
                                       true,
                                       false,
                                       {0.0f, 0.0f},
                                       {0.0f, 0.0f, 0.0f},
                                       {1.0f, 1.0f, 1.0f},
                                       0.0f,
                                       0.0f };

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);

    // 1. setup reconstructor
    slicerecon::Reconstructor recon(params, geom);

    // 2. listen to projection stream projection callback, push to projection stream all raw data
    auto proj = slicerecon::ProjectionServer(hostname, port, socket_type);
    proj.start(recon, params);

    // 3. connect with visualization server
    auto viz = slicerecon::VisualizationServer("slicerecon test",
                                               "tcp://"s + gui_hostname + ":5555"s,
                                               "tcp://"s + gui_hostname + ":5556"s);
    viz.set_slice_callback([&](auto x, auto idx) { return recon.reconstructSlice(x); });
    recon.addListener(&viz);

    auto plugin_one = slicerecon::plugin("tcp://*:5650", "tcp://localhost:5651");
    plugin_one.set_slice_callback(
        [](auto shape, auto data, auto index) -> std::pair<std::array<int32_t, 2>, std::vector<float>> {
            for (auto& x : data) {
                if (x <= 3) {
                    x = 0;
                }
                else {
                    x = 17;
                }
            }

            return {shape, data};
        }
    );

    auto plugin_two = slicerecon::plugin(
        "tcp://*:5651", "tcp://"s + gui_hostname + ":5555"s);
    plugin_two.set_slice_callback(
        [](auto shape, auto data,
        auto index) -> std::pair<std::array<int32_t, 2>, std::vector<float>> {
            for (auto& x : data) {
                (void)x;
            }

            return {shape, data};
        }
    );

    if (plugin) {
        viz.register_plugin("tcp://localhost:5650");
        plugin_one.serve();
        plugin_two.serve();
    }

    if (py_plugin) {
        std::cout << "Registering plugin at 5652\n";
        viz.register_plugin("tcp://localhost:5652");
    }

    if (bench) {
        slicerecon::util::bench.register_listener(&viz);
        slicerecon::util::bench.enable();
    }
    viz.serve();

    return 0;
}
