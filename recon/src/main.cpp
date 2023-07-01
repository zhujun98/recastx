#include <functional>
#include <memory>
#include <numeric>
#include <string>

#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "recon/application.hpp"
#include "recon/reconstructor.hpp"
#include "common/config.hpp"

namespace po = boost::program_options;


std::pair<uint32_t, uint32_t> parseDownsampleFactor(
        const po::variable_value& downsample_row, 
        const po::variable_value& downsample_col, 
        const po::variable_value& downsample) {
    uint32_t row = downsample_row.empty() ? downsample.as<uint32_t>() : downsample_row.as<uint32_t>();
    uint32_t col = downsample_col.empty() ? downsample.as<uint32_t>() : downsample_col.as<uint32_t>();
    return {row, col};
}

int main(int argc, char** argv) {

    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    po::options_description general_desc("General options");
    general_desc.add_options()
        ("help,h", "print help message")
    ;

    po::options_description connection_desc("ZMQ options");
    connection_desc.add_options()
        ("daq-host", po::value<std::string>()->default_value("localhost"), 
         "hostname of the DAQ server")
        ("daq-port", po::value<int>()->default_value(9667),
         "ZMQ socket port of the DAQ server")
        ("daq-socket", po::value<std::string>()->default_value("pull"),
         "ZMQ socket type of the DAQ server. Options: sub/pull")
        ("rpc-port", po::value<int>()->default_value(9971),
         "port of the gRPC server."
         "At TOMCAT, the valid port range is [9970, 9979]")
    ;

    po::options_description geometry_desc("Geometry options");
    bool cone_beam = false;
    geometry_desc.add_options()
        ("cols", po::value<size_t>()->default_value(2016),
         "detector width in pixels")
        ("rows", po::value<size_t>()->default_value(1200),
         "detector height in pixels")
        ("downsample", po::value<uint32_t>()->default_value(1),
         "downsampling factor along both the row and the column. It will be "
         "overwirtten if 'downsample-col' or 'downsample-row' is given")
        ("downsample-col", po::value<uint32_t>(),
         "downsampling factor along the column")
        ("downsample-row", po::value<uint32_t>(),
         "downsampling factor along the row")
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
        ("preview-size", po::value<size_t>(),
         "size of the cubic reconstructed volume for preview.")
        ("threads", po::value<uint32_t>()->default_value(8),
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

    auto daq_hostname = opts["daq-host"].as<std::string>();
    auto daq_port = opts["daq-port"].as<int>();
    auto daq_socket_type = opts["daq-socket"].as<std::string>();
    auto rpc_port = opts["rpc-port"].as<int>();

    auto [downsampling_row, downsampling_col] = parseDownsampleFactor(
        opts["downsample-row"], opts["downsample-col"], opts["downsample"]);
    auto num_rows = opts["rows"].as<size_t>();
    auto num_cols = opts["cols"].as<size_t>();
    auto num_angles = opts["angles"].as<size_t>();
    auto minx = opts["minx"].empty() ? std::nullopt : std::optional<float>{opts["minx"].as<float>()}; 
    auto maxx = opts["maxx"].empty() ? std::nullopt : std::optional<float>{opts["maxx"].as<float>()}; 
    auto miny = opts["miny"].empty() ? std::nullopt : std::optional<float>{opts["miny"].as<float>()}; 
    auto maxy = opts["maxy"].empty() ? std::nullopt : std::optional<float>{opts["maxy"].as<float>()}; 
    auto minz = opts["minz"].empty() ? std::nullopt : std::optional<float>{opts["minz"].as<float>()}; 
    auto maxz = opts["maxz"].empty() ? std::nullopt : std::optional<float>{opts["maxz"].as<float>()}; 

    auto slice_size = opts["slice-size"].empty() ? std::nullopt 
                                                 : std::optional<size_t>(opts["slice-size"].as<size_t>());
    auto preview_size = opts["preview-size"].empty() ? std::nullopt 
                                                     : std::optional<size_t>(opts["preview-size"].as<size_t>());
    auto num_threads = opts["threads"].as<uint32_t>();
    auto num_darks = opts["darks"].as<size_t>();
    auto num_flats = opts["flats"].as<size_t>();
    auto raw_buffer_size = opts["raw-buffer-size"].as<size_t>();

    auto filter_name = opts["filter"].as<std::string>();

    auto pixel_size = opts["pixel-size"].as<float>();
    auto lambda = opts["lambda"].as<float>();
    auto delta = opts["delta"].as<float>();
    auto beta = opts["beta"].as<float>();
    auto distance = opts["distance"].as<float>();

    recastx::DaqClientConfig daq_client_cfg {daq_port, daq_hostname, daq_socket_type};
    recastx::RpcServerConfig rpc_server_cfg {rpc_port};
    recastx::ImageprocParams imageproc_params {num_threads, downsampling_col, downsampling_row};
    recastx::recon::Application app(raw_buffer_size, imageproc_params, daq_client_cfg, rpc_server_cfg);

    app.setFlatFieldCorrectionParams(num_darks, num_flats);
    app.setFilterParams(gaussian_lowpass_filter, filter_name);
    if (retrieve_phase) app.setPaganinParams(pixel_size, lambda, delta, beta, distance);
    app.setProjectionGeometry(cone_beam ? recastx::BeamShape::CONE : recastx::BeamShape::PARALELL, 
                              num_cols, num_rows, 1.0f, 1.0f, 0.0f, 0.0f, num_angles);
    app.setReconGeometry(slice_size, preview_size, minx, maxx, miny, maxy, minz, maxz);

    app.runForEver();

    return 0;
}
