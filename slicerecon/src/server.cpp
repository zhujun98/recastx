#include <functional>
#include <memory>
#include <numeric>
#include <string>

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "slicerecon/reconstructor.hpp"
#include "slicerecon/data_receiver.hpp"
#include "slicerecon/broker.hpp"
#include "slicerecon/utils.hpp"

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
        ("data-socket", po::value<std::string>()->default_value("pull"),
         "ZMQ socket type of the data source server. Options: sub/pull")
        ("gui-host", po::value<std::string>()->default_value("localhost"),
         "hostname of the GUI server")
        ("gui-port", po::value<int>()->default_value(5555),
         "First ZMQ socket port of the GUI server. The second port has an increment of 1.")
    ;

    po::options_description geometry_desc("Geometry options");
    bool cone_beam = false;
    geometry_desc.add_options()
        ("rows", po::value<int>()->default_value(1200),
         "detector height in pixels")
        ("cols", po::value<int>()->default_value(2016),
         "detector width in pixels")
    ;

    po::options_description reconstruction_desc("Reconstruction options");
    bool continuous_mode = false;
    bool retrieve_phase = false;
    bool tilt = false;
    bool gaussian_pass = false;
    reconstruction_desc.add_options()
        ("slice-size", po::value<int>()->default_value(-1),
         "size of the square reconstructed slice in pixels. Default to detector columns.")
        ("preview-size", po::value<int>()->default_value(128),
         "size of the cubic reconstructed volume for preview. Default to slice size.")
        ("threads", po::value<int>()->default_value(8),
         "number of threads used for data processing")
        ("darks", po::value<int>()->default_value(10),
         "number of dark images")
        ("flats", po::value<int>()->default_value(10),
         "number of flat images")
        ("projections", po::value<int>()->default_value(128),
         "number of projections")
        ("group-size", po::value<int>()->default_value(-1),
         "group size for projection processing in the alternative mode. "
         "Default to number of projections.")
        ("continuous-mode", po::bool_switch(&continuous_mode),
         "switch reconstructor to continuous mode from the default alternating mode")
        ("retrieve-phase", po::bool_switch(&retrieve_phase),
         "switch to Paganin filter")
        ("tilt", po::bool_switch(&tilt),
         "...")
        ("filter", po::value<std::string>()->default_value("shepp"),
         "Supported filters are: shepp (Shepp-Logan), ramlak (Ram-Lak)")
        ("gaussian", po::bool_switch(&gaussian_pass),
         "enable Gaussian low pass filter")
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
        add(geometry_desc).
        add(reconstruction_desc).
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

    auto rows = opts["rows"].as<int>();
    auto cols = opts["cols"].as<int>();

    auto slice_size = opts["slice-size"].as<int>();
    if (slice_size <= 0) slice_size = cols;
    auto preview_size = opts["preview-size"].as<int>();
    auto num_threads = opts["threads"].as<int>();
    auto num_darks = opts["darks"].as<int>();
    auto num_flats = opts["flats"].as<int>();
    auto num_projections = opts["projections"].as<int>();
    auto group_size = opts["group-size"].as<int>();
    if (group_size <= 0) group_size = num_projections;
    if (num_projections < group_size) {
        throw std::invalid_argument(
            "'projections' ("s + std::to_string(num_projections) + 
            ") is smaller than 'group_size' ("s + std::to_string(group_size) + ")"s);
    }

    auto recon_mode = continuous_mode ? slicerecon::ReconstructMode::continuous : 
                                        slicerecon::ReconstructMode::alternating;
    auto filter_name = opts["filter"].as<std::string>();

    auto pixel_size = opts["pixel-size"].as<float>();
    auto lambda = opts["lambda"].as<float>();
    auto delta = opts["delta"].as<float>();
    auto beta = opts["beta"].as<float>();
    auto distance = opts["distance"].as<float>();

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);

    float hdw = cols / 2.f; // half detector width
    std::array<float, 3> volume_min_point {-hdw, -hdw, -hdw};
    std::array<float, 3> volume_max_point {hdw, hdw, hdw};
    std::array<float, 2> detector_size {1.0f, 1.0f};
    float source_origin = 0.f;
    float origin_det = 0.f;

    bool vec_geometry = false;

    // 1. set up reconstructor
    auto recon = std::make_shared<slicerecon::Reconstructor>(rows, cols, num_threads);

    recon->initialize(num_darks, num_flats, num_projections, group_size, preview_size, recon_mode);

    if (retrieve_phase) recon->initPaganin(pixel_size, lambda, delta, beta, distance);

    recon->initFilter(filter_name, gaussian_pass);

    // set up solver

    // TODO:: receive/create angles in different ways.
    auto angles = slicerecon::utils::defaultAngles(num_projections);
    if (cone_beam) {
        recon->setSolver(std::make_unique<slicerecon::ConeBeamSolver>(
            rows, cols, angles, volume_min_point, volume_max_point, preview_size, slice_size,
            vec_geometry, detector_size, source_origin, origin_det, recon_mode
        ));
    } else {
        recon->setSolver(std::make_unique<slicerecon::ParallelBeamSolver>(
            rows, cols, angles, volume_min_point, volume_max_point, preview_size, slice_size, 
            vec_geometry, detector_size, recon_mode
        ));
    }

    // set up data bridges

    auto data_receiver = slicerecon::DataReceiver(
        "tcp://"s + data_hostname + ":"s + std::to_string(data_port),
        data_socket_type,
        recon);
    data_receiver.start();

    auto broker = slicerecon::Broker(
        "tcp://"s + gui_hostname + ":"s + std::to_string(gui_port),
        "tcp://"s + gui_hostname + ":"s + std::to_string(gui_port + 1),
        recon);
    broker.start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
