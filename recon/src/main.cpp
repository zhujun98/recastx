#include <functional>
#include <memory>
#include <numeric>
#include <string>

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "recon/application.hpp"
#include "recon/reconstructor.hpp"
#include "recon/preprocessing.hpp"
#include "tomcat/tomcat.hpp"

namespace po = boost::program_options;


std::pair<float, float> parseReconstructedVolumeBoundary(
        const po::variable_value& min_val, const po::variable_value& max_val, int size) {
    float min_v = min_val.empty() ? - size / 2.f : min_val.as<float>();
    float max_v = max_val.empty() ?   size / 2.f : max_val.as<float>();
    if (min_v >= max_v) throw std::invalid_argument(
        "Minimum of volume coordinate must be smaller than maximum of volume coordinate");
    return {min_v, max_v};
}

int main(int argc, char** argv) {

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

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
         "First ZMQ socket port of the GUI server. "
         "At TOMCAT, the valid port range is [9970, 9979]")
        ("gui-port2", po::value<int>()->default_value(9970),
         "Second ZMQ socket port of the GUI server. "
         "If it is equal to the first one, it will be increased by 1. "
         "At TOMCAT, the valid port range is [9970, 9979]")
    ;

    po::options_description geometry_desc("Geometry options");
    bool cone_beam = false;
    geometry_desc.add_options()
        ("cols", po::value<size_t>()->default_value(2016),
         "detector width in pixels")
        ("rows", po::value<size_t>()->default_value(1200),
         "detector height in pixels")
        ("downsample-col", po::value<size_t>()->default_value(1))
        ("downsample-row", po::value<size_t>()->default_value(1))
        ("angles", po::value<size_t>()->default_value(128),
         "number of projections per scan")
        ("minx", po::value<float>(),
         "minimal X-coordinate of the reconstructed volume")
        ("maxx", po::value<float>(),
         "maximal X-coordinate of the reconstructed volume")
        ("miny", po::value<float>(),
         "minimal Y-coordinate of the reconstructed volume")
        ("maxy", po::value<float>(),
         "maximal Y-coordinate of the reconstructed volume")
        ("minz", po::value<float>(),
         "minimal Z-coordinate of the reconstructed volume")
        ("maxz", po::value<float>(),
         "maximal Z-coordinate of the reconstructed volume")
    ;

    po::options_description reconstruction_desc("Reconstruction options");
    bool retrieve_phase = false;
    bool tilt = false;
    bool gaussian_lowpass_filter = false;
    reconstruction_desc.add_options()
        ("slice-size", po::value<size_t>(),
         "size of the square reconstructed slice in pixels. Default to detector columns.")
        ("preview-size", po::value<size_t>()->default_value(128),
         "size of the cubic reconstructed volume for preview.")
        ("threads", po::value<size_t>()->default_value(8),
         "number of threads used for data processing")
        ("darks", po::value<size_t>()->default_value(10),
         "number of required dark images")
        ("flats", po::value<size_t>()->default_value(10),
         "number of required flat images")
        ("raw-buffer-size", po::value<size_t>()->default_value(10),
         "maximum number of projection groups to be cached in the memory buffer")
        ("retrieve-phase", po::bool_switch(&retrieve_phase),
         "switch to Paganin filter")
        ("tilt", po::bool_switch(&tilt),
         "...")
        ("filter", po::value<std::string>()->default_value("shepp"),
         "Supported filters are: shepp (Shepp-Logan), ramlak (Ram-Lak)")
        ("gaussian-lowpass-filter", po::bool_switch(&gaussian_lowpass_filter),
         "enable Gaussian low-pass filter (not verified)")
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
    auto gui_port2 = opts["gui-port2"].as<int>();
    if (gui_port2 == gui_port) gui_port2 += 1;

    auto downsample_col = opts["downsample-col"].as<size_t>();
    auto downsample_row = opts["downsample-row"].as<size_t>();
    auto num_cols = opts["cols"].as<size_t>() / downsample_col;
    auto num_rows = opts["rows"].as<size_t>() / downsample_row;
    auto num_angles = opts["angles"].as<size_t>();
    auto [min_x, max_x] = parseReconstructedVolumeBoundary(opts["minx"], opts["maxx"], num_cols);
    auto [min_y, max_y] = parseReconstructedVolumeBoundary(opts["miny"], opts["maxy"], num_cols);
    auto [min_z, max_z] = parseReconstructedVolumeBoundary(opts["minz"], opts["maxz"], num_rows);
    size_t slice_size = opts["slice-size"].empty() ? num_cols : opts["slice-size"].as<size_t>();
    auto preview_size = opts["preview-size"].as<size_t>();
    auto num_threads = opts["threads"].as<size_t>();
    auto num_darks = opts["darks"].as<size_t>();
    auto num_flats = opts["flats"].as<size_t>();
    auto raw_buffer_size = opts["raw-buffer-size"].as<size_t>();

    auto filter_name = opts["filter"].as<std::string>();

    auto pixel_size = opts["pixel-size"].as<float>();
    auto lambda = opts["lambda"].as<float>();
    auto delta = opts["delta"].as<float>();
    auto beta = opts["beta"].as<float>();
    auto distance = opts["distance"].as<float>();

    auto app = std::make_shared<Application>(raw_buffer_size, num_threads);

    app->init(num_rows, num_cols, num_angles, num_darks, num_flats, downsample_row, downsample_col);

    if (retrieve_phase) app->initPaganin(
        {pixel_size, lambda, delta, beta, distance}, num_cols, num_rows);

    app->initFilter({filter_name, gaussian_lowpass_filter}, num_cols, num_rows);

    float half_slice_height = 0.5f * (max_z - min_z) / preview_size;
    float z0 = 0.5f * (max_z + min_z);
    app->initReconstructor(
        cone_beam, 
        {num_cols, num_rows, 1.f, 1.f, defaultAngles(num_angles), 0.0f, 0.0f}, 
        {slice_size, slice_size, 1, min_x, max_x, min_y, max_y, z0 - half_slice_height, z0 + half_slice_height},
        {preview_size, preview_size, preview_size, min_x, max_x, min_y, max_y, min_z, max_z}
    );
    
    app->initConnection({data_hostname, data_port, data_socket_type}, {gui_port, gui_port2});

    app->runForEver();

    return 0;
}
