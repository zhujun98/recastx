#include <functional>
#include <numeric>
#include <string>

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "slicerecon/reconstructor.hpp"
#include "slicerecon/projection_server.hpp"
#include "slicerecon/visualization_server.hpp"

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
        ("data-host", po::value<std::string>()->default_value("localhost"), 
         "hostname of the data source server")
        ("data-port", po::value<int>()->default_value(5558),
         "ZMQ socket port of the data source server")
        ("data-socket", po::value<std::string>()->default_value("sub"),
         "ZMQ socket type of the data source server. Options: sub/pull")
        ("gui-host", po::value<std::string>()->default_value("localhost"),
         "hostname of the GUI server")
        ("gui-port", po::value<int>()->default_value(5555),
         "First ZMQ socket port of the GUI server. The second port has an increment of 1.")
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
         "switch reconstructor to continuous mode from the default alternating mode")
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

    po::options_description all_desc(
        "Allowed options for TOMCAT live 3D reconstruction server");
    all_desc.
        add(general_desc).
        add(connection_desc).
        add(reconstruction_desc).
        add(geometry_desc).
        add(paganin_desc);

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, all_desc), opts);
    po::notify(opts);

    if (opts.count("help")) {
        std::cout << all_desc << "\n";
        return 0;
    }

    auto data_hostname = opts["data-host"].as<std::string>();
    auto data_port = opts["data-port"].as<int>();
    auto data_socket_type = parseSocketType(opts["data-socket"].as<std::string>());
    auto gui_hostname = opts["gui-host"].as<std::string>();
    auto gui_port = opts["gui-port"].as<int>();

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

    slicerecon::PaganinSettings paganin {pixel_size, lambda, delta, beta, distance};

    slicerecon::Settings params { slice_size, 
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

    slicerecon::Geometry geom { rows,
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
    auto proj = slicerecon::ProjectionServer(data_hostname, data_port, data_socket_type);
    proj.start(recon, params);

    // 3. connect with visualization server
    auto viz = slicerecon::VisualizationServer(
        "tomcat-live GUI server",
        "tcp://"s + gui_hostname + ":"s + std::to_string(gui_port),
        "tcp://"s + gui_hostname + ":"s + std::to_string(gui_port + 1));
    viz.set_slice_callback([&](auto x, auto idx) { return recon.reconstructSlice(x); });
    recon.addListener(&viz);

    viz.serve();

    return 0;
}
