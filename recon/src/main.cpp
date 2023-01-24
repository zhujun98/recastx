#include <functional>
#include <memory>
#include <numeric>
#include <string>

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "recon/server.hpp"
#include "recon/reconstructor.hpp"
#include "recon/utils.hpp"
#include "tomcat/tomcat.hpp"

namespace po = boost::program_options;

namespace tomcat::recon {

std::array<float, 3> makeVolumeMinmaxPoint(const po::variable_value& point, float v) {
    if (point.empty()) return {v, v, v};

    auto pv = point.as<std::vector<float>>();
    if (pv.size() == 1) return {pv[0], pv[0], pv[0]};
    if (pv.size() == 3) return {pv[0], pv[1], pv[2]};
    
    throw std::invalid_argument(
        "Length of volume min/max point vector must be either 1 or 3");
}

} // tomcat::recon

int main(int argc, char** argv)
{
    using namespace tomcat::recon;

    po::options_description general_desc("General options");
    general_desc.add_options()
        ("help,h", "print help message")
    ;

    po::options_description connection_desc("ZMQ options");
    connection_desc.add_options()
        ("data-host", po::value<std::string>()->default_value("localhost"), 
         "hostname of the data source server")
        ("data-port", po::value<int>()->default_value(9667),
         "ZMQ socket port of the data source server")
        ("data-socket", po::value<std::string>()->default_value("pull"),
         "ZMQ socket type of the data source server. Options: sub/pull")
        ("gui-port", po::value<int>()->default_value(9970),
         "First ZMQ socket port of the GUI server. The second port has an increment of 1. "
         "The valid port range is [9970, 9979]")
    ;

    po::options_description geometry_desc("Geometry options");
    bool cone_beam = false;
    geometry_desc.add_options()
        ("rows", po::value<int>()->default_value(1200),
         "detector height in pixels")
        ("cols", po::value<int>()->default_value(2016),
         "detector width in pixels")
        ("volume-min-point", po::value<std::vector<float>>()->multitoken(), 
         "minimal (X, Y, Z)-coordinate in the volume window.")
        ("volume-max-point", po::value<std::vector<float>>()->multitoken(), 
         "maximal (X, Y, Z)-coordinate in the volume window.")
    ;

    po::options_description reconstruction_desc("Reconstruction options");
    bool retrieve_phase = false;
    bool tilt = false;
    bool gaussian_pass = false;
    reconstruction_desc.add_options()
        ("slice-size", po::value<int>()->default_value(-1),
         "size of the square reconstructed slice in pixels. Default to detector columns.")
        ("preview-size", po::value<int>()->default_value(128),
         "size of the cubic reconstructed volume for preview.")
        ("threads", po::value<int>()->default_value(8),
         "number of threads used for data processing")
        ("darks", po::value<int>()->default_value(10),
         "number of required dark images")
        ("flats", po::value<int>()->default_value(10),
         "number of required flat images")
        ("angles", po::value<int>()->default_value(128),
         "number of projections per scan")
        ("buffer-size", po::value<int>()->default_value(10),
         "maximum number of projection groups to be cached in the memory buffer")
        ("retrieve-phase", po::bool_switch(&retrieve_phase),
         "switch to Paganin filter")
        ("tilt", po::bool_switch(&tilt),
         "...")
        ("filter", po::value<std::string>()->default_value("shepp"),
         "Supported filters are: shepp (Shepp-Logan), ramlak (Ram-Lak)")
        ("gaussian", po::bool_switch(&gaussian_pass),
         "enable Gaussian low pass filter (not verified)")
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
    auto data_socket_type = opts["data-socket"].as<std::string>();
    auto gui_port = opts["gui-port"].as<int>();

    auto num_rows = opts["rows"].as<int>();
    auto num_cols = opts["cols"].as<int>();
    auto vminp = opts["volume-min-point"];
    auto vmaxp = opts["volume-max-point"];
    auto volume_min_point = makeVolumeMinmaxPoint(vminp, -num_cols / 2.f);
    auto volume_max_point = makeVolumeMinmaxPoint(vmaxp, num_cols / 2.f);

    auto slice_size = opts["slice-size"].as<int>();
    if (slice_size <= 0) slice_size = num_cols;
    auto preview_size = opts["preview-size"].as<int>();
    auto num_threads = opts["threads"].as<int>();
    auto num_darks = opts["darks"].as<int>();
    auto num_flats = opts["flats"].as<int>();
    auto num_angles = opts["angles"].as<int>();
    auto buffer_size = opts["buffer-size"].as<int>();

    auto filter_name = opts["filter"].as<std::string>();

    auto pixel_size = opts["pixel-size"].as<float>();
    auto lambda = opts["lambda"].as<float>();
    auto delta = opts["delta"].as<float>();
    auto beta = opts["beta"].as<float>();
    auto distance = opts["distance"].as<float>();

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);

    // 1. set up server
    auto server = std::make_shared<Server>(num_threads);

    server->init(num_rows, num_cols, num_angles, num_darks, num_flats, slice_size, preview_size, buffer_size);

    if (retrieve_phase) server->initPaganin(pixel_size, lambda, delta, beta, distance, num_cols, num_rows);

    server->initFilter(filter_name, num_rows, num_cols, gaussian_pass);

    server->setReconstructor(tomcat::recon::createReconstructor(
        cone_beam, num_rows, num_cols, num_angles, 1.f, 1.f, 0.0f, 0.0f, 
        slice_size, preview_size, volume_min_point, volume_max_point));
    
    server->initConnection({data_hostname, data_port, data_socket_type}, {gui_port, gui_port + 1});

    server->runForEver();

    // set up data bridges

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
